defmodule Administrator do
  @moduledoc false
  use AMQP
  @behaviour Worker

  require Logger
  alias Message

  ##############################################################################
  # Doctor API
  ##############################################################################

  @spec send_info(name :: Atom.t() | Pit.t()) :: :ok
  def send_info(name \\ __MODULE__) do
    Process.send_after(name, :input, 1000)
  end


  ##############################################################################
  # Worker API
  ##############################################################################

  def start_link(options \\ [name: __MODULE__]) do
    Worker.start_link(__MODULE__, options)
  end

  def stop(name \\ __MODULE__, reason) do
    Worker.stop(name, reason)
  end

  def ack(%Message{attributes: %{delivery_tag: tag}} = message) do
    Logger.debug("Message successfully processed. Tag: #{tag}")
    Worker.ack(message)
  end

  def reject(%Message{attributes: %{delivery_tag: tag}} = message, requeue \\ true) do
    Logger.info("Rejecting message, tag: #{tag}, requeue: #{requeue}")
    Worker.reject(message, requeue)
  end

  def publish(publisher \\ __MODULE__, exchange, message, routing_key) do
    Logger.info("Publishing message #{inspect(message)}")
    Worker.publish(publisher, exchange, message, routing_key)
  end

  ##############################################################################
  # Worker callbacks
  ##############################################################################

  def init() do
    [
      tag: worker_tag(),
      prefetch_count: "1",
      uri: "amqp://guest:guest@localhost:5672",
      concurrency: true,
      log_queue: "log_queue",
      admin_exchange: "admin",
      info_key: "info",
      log_key: "log",
    ]
  end

  def handle_message(%Message{attributes: attributes, state: %{config: config}} = message) do
    name = config[:name]
    admin_exchange = config[:admin_exchange]
    case attributes.exchange do
      ^admin_exchange ->
        Logger.info("[#{name}] Received log: #{message.payload}")
      _ ->
        Logger.info("[#{name}] Received message: #{inspect(message)}")
    end
    ack(message)
  rescue
    exception ->
      Logger.error(Exception.format(:error, exception, System.stacktrace()))
      reject(message, false)
  end

  def handle_input(%{chan: channel, config: config}) do
    IO.puts("Enter message:")
    msg = IO.read(:stdio, :line) |> String.trim()
    :ok = AMQP.Basic.publish(
      channel,
      config[:admin_exchange],
      config[:info_key],
      msg
    )
  end

  def worker_tag() do
    "#{__MODULE__}_#{UUID.uuid1()}"
  end

  def setup_queues(%{chan: chan, config: config} = state) do
    log_queue = config[:log_queue]
    admin_exchange = config[:admin_exchange]
    log_key = config[:log_key]
    prefetch_count = String.to_integer(config[:prefetch_count])

    :ok = Basic.qos(chan, prefetch_count: prefetch_count)
    {:ok, _} = Queue.declare(chan, log_queue, auto_delete: true)
    :ok = Exchange.topic(chan, admin_exchange, durable: true)
    :ok = Queue.bind(chan, log_queue, admin_exchange, routing_key: log_key)
    {:ok, _} = Basic.consume(chan, log_queue)
    state
  end

  ##############################################################################
  # Helpers
  ##############################################################################

end
