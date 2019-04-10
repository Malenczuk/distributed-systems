defmodule Worker do
  @moduledoc false
  use GenServer
  use AMQP

  alias Message

  require Logger

  ##############################################################################
  # Worker callbacks
  ##############################################################################

  @callback init() :: [
                        tag: String.t(),
                        prefetch_count: String.t(),
                        uri: String.t(),
                        concurrency: Boolean.t(),
                        retry_delay_function: Function.t(),
                        reconnect: Boolean.t(),
                      ]

  @callback worker_tag() :: String.t()

  @callback handle_message(message :: Message.t()) :: :ok

  @callback handle_input(state :: Keyword.t() ) :: :ok

  @callback setup_queues(state :: Keyword.t() ) :: Keyword.t()


  ##############################################################################
  # Worker API
  ##############################################################################

  @spec start_link(module :: Module.t(), options :: Keyword.t()) :: {:ok, Pid.t()} | {:error, Any.t()}
  def start_link(module, options \\ []) do
    Keyword.put_new(options, :name, module)
    GenServer.start_link(__MODULE__, %{module: module, config: options}, options)
  end

  @spec stop(name :: Atom.t() | Pit.t(), reason :: Any.t()) :: :ok
  def stop(name, reason) do
    GenServer.stop(name, reason)
  end

  @spec ack(message :: Message.t()) :: :ok
  def ack(%Message{state: %{chan: channel}, attributes: %{delivery_tag: tag}}) do
    Basic.ack(channel, tag)
  end

  @spec reject(message :: Message.t(), requeue :: Boolean.t()) :: :ok
  def reject(%Message{state: %{chan: channel}, attributes: %{delivery_tag: tag}}, requeue \\ false) do
    Basic.reject(channel, tag, requeue: requeue)
  end

  @spec publish(
          publisher :: Atom.t() | Pid.t(),
          exchange :: String.t(),
          message :: Binary.t(),
          routing_key :: String.t(),
          metadata :: Keyword.t()
        ) :: :ok
  def publish(publisher, exchange, message, routing_key \\ "", metadata \\ []) do
    GenServer.call(publisher, {:publish, exchange, message, routing_key, metadata})
  end

  ##############################################################################
  # GenServer callbacks
  ##############################################################################

  @impl GenServer
  def init(%{module: module, config: config} = initial_state) do
    Process.flag(:trap_exit, true)
    config =
      apply(module, :init, [])
      |> Keyword.merge(config)
    Logger.debug("#{inspect(config)}")
    state =
      initial_state
      |> Map.put(:config, config)
      |> Map.put(:reconnect_attempt, 0)

    send(self(), :init)

    {:ok, state}
  end

  @impl GenServer
  def handle_call({:recover, requeue}, _from, %{chan: channel} = state) do
    {:reply, Basic.recover(channel, requeue: requeue), state}
  end

  @impl GenServer
  def handle_call({:publish, exchange, msg, key, metadata}, _from, %{chan: channel} = state) do
    result = Basic.publish(channel, exchange, key, msg, metadata)
    {:reply, result, state}
  end

  @impl GenServer
  def handle_info(:input, %{module: module} = state) do
    apply(module, :handle_input, [state])
    {:noreply, state}
  end

  @impl GenServer
  def handle_info(:init, state) do
    state =
      state
      |> get_connection()
      |> open_channel()
      |> setup()


    {:noreply, state}
  end

  @impl GenServer
  def handle_info({:DOWN, _ref, :process, _pid, reason}, %{module: module, config: config} = state) do
    Logger.info("[#{module}]: RabbitMQ connection is down! Reason: #{inspect(reason)}")

    config
    |> Keyword.get(:reconnect, true)
    |> handle_reconnect(state)
  end

  @impl GenServer
  def handle_info({:basic_consume_ok, %{consumer_tag: consumer_tag}}, %{module: module} = state) do
    Logger.info("[#{module}]: Broker confirmed consumer with tag #{consumer_tag}")
    {:noreply, state}
  end

  @impl GenServer
  def handle_info({:basic_cancel, %{consumer_tag: consumer_tag}}, %{module: module} = state) do
    Logger.warn("[#{module}]: The consumer was unexpectedly cancelled, tag: #{consumer_tag}")
    {:stop, :cancelled, state}
  end

  @impl GenServer
  def handle_info({:basic_cancel_ok, %{consumer_tag: consumer_tag}}, %{module: module} = state) do
    Logger.info("[#{module}]: Consumer was cancelled, tag: #{consumer_tag}")
    {:noreply, state}
  end

  @impl GenServer
  def handle_info({:basic_deliver, payload, attributes}, %{module: module, config: config} = state) do
    %{delivery_tag: tag, routing_key: routing_key, redelivered: redelivered} = attributes
    Logger.debug("[#{module}]: Received message. Tag: #{tag}, routing key: #{routing_key}, redelivered: #{redelivered}")

    if redelivered do
      Logger.debug("[#{module}]: Redelivered payload for message. Tag: #{tag}, payload: #{payload}")
    end

    handle_message(payload, attributes, state, Keyword.get(config, :concurrency, true))

    {:noreply, state}
  end

  @impl GenServer
  def terminate(:connection_closed = reason, %{module: module}) do
    # Since connection has been closed no need to clean it up
    Logger.debug("[#{module}]: Terminating worker, reason: #{inspect(reason)}")
  end

  @impl GenServer
  def terminate(reason, %{module: module, conn: conn, chan: chan}) do
    Logger.debug("[#{module}]: Terminating worker, reason: #{inspect(reason)}")
    Channel.close(chan)
    Connection.close(conn)
  end

  @impl GenServer
  def terminate(reason, %{module: module}) do
    Logger.debug("[#{module}]: Terminating worker, reason: #{inspect(reason)}")
  end

  ##############################################################################
  # Helpers
  ##############################################################################

  defp handle_message(payload, attributes, %{module: module} = state, false) do
    message = Message.create(attributes, payload, state)
    apply(module, :handle_message, [message])
  end

  defp handle_message(payload, attributes, %{module: module} = state, true) do
    spawn(fn ->
      message = Message.create(attributes, payload, state)
      apply(module, :handle_message, [message])
    end)
  end

  defp handle_reconnect(false, %{module: module} = state) do
    Logger.info("[#{module}]: Reconnection is disabled. Terminating worker.")
    {:stop, :connection_closed, state}
  end

  defp handle_reconnect(_, state) do
    new_state =
      state
      |> Map.put(:reconnect_attempt, 0)
      |> get_connection()
      |> open_channel()
      |> setup()

    {:noreply, new_state}
  end

  defp get_connection(%{config: config, module: module, reconnect_attempt: attempt} = state) do
    case Connection.open(config[:uri]) do
      {:ok, conn} ->
        Process.monitor(conn.pid)
        Map.put(state, :conn, conn)

      {:error, e} ->
        Logger.error(
          "[#{module}]: Failed to connect to RabbitMQ with settings: " <>
          "#{inspect(strip_key(config, :uri))}, reason #{inspect(e)}"
        )

        retry_delay_fn = config[:retry_delay_function] || (&linear_delay/1)
        next_attempt = attempt + 1
        retry_delay_fn.(next_attempt)

        state
        |> Map.put(:reconnect_attempt, next_attempt)
        |> get_connection()
    end
  end

  defp open_channel(%{conn: conn} = state) do
    {:ok, chan} = Channel.open(conn)
    Map.merge(state, %{chan: chan})
  end

  defp setup(%{module: module} = state) do
    apply(module, :setup_queues, [state])
  end

  defp strip_key(keyword_list, key) do
    keyword_list
    |> Keyword.delete(key)
    |> Keyword.put(key, "[FILTERED]")
  end

  defp linear_delay(attempt), do: :timer.sleep(attempt * 1_000)

  ##############################################################################
  ##############################################################################
  ##############################################################################
end