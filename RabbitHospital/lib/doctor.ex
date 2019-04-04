defmodule Doctor do
  @moduledoc false
  use AMQP
  @behaviour Worker

  require Logger
  alias Message

  ##############################################################################
  # Doctor API
  ##############################################################################

  @spec request(name :: Atom.t() | Pit.t()) :: :ok
  def request(name \\ __MODULE__) do
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

  def publish(publisher \\ __MODULE__, exchange, message, routing_key \\ "", metadata \\ []) do
    Logger.info("Publishing message #{inspect(message)}")
    Worker.publish(publisher, exchange, message, routing_key, metadata)
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
      hospital_exchange: "hospital",
      admin_exchange: "admin",
      info_key: "info",
      log_key: "log",
    ]
  end

  def handle_message(%Message{attributes: attributes, state: %{config: config}} = message) do
    name = config[:name]
    hospital_exchange = config[:hospital_exchange]
    admin_exchange = config[:admin_exchange]
    case attributes.exchange do
      ^hospital_exchange ->
        Logger.info("[#{name}] Received results: #{message.payload}")
      ^admin_exchange ->
        Logger.info("[#{name}] Received info: #{message.payload}")
      _ ->
        Logger.info("[#{name}] Received message: #{inspect(message)}")
    end
    ack(message)
  rescue
    exception ->
      Logger.error(Exception.format(:error, exception, System.stacktrace()))
      reject(message, false)
  end

  def handle_input(%{chan: chan, config: config}) do
    IO.puts("Enter the name of the patient:")
    name = IO.read(:stdio, :line) |> String.trim()
    IO.puts("Enter the type of diagnosis:")
    type = IO.read(:stdio, :line) |> String.trim()
    publish_and_log(chan, type, name, config)
  end

  def worker_tag() do
    "#{__MODULE__}_#{UUID.uuid1()}"
  end

  def setup_queues(%{chan: chan, config: config} = state) do
    tag = config[:tag]
    response_queue = "#{tag}_queue"
    hospital_exchange = config[:hospital_exchange]
    admin_exchange = config[:admin_exchange]
    info_key = config[:info_key]
    prefetch_count = String.to_integer(config[:prefetch_count])

    :ok = Basic.qos(chan, prefetch_count: prefetch_count)
    {:ok, _} = Queue.declare(chan, response_queue)
    :ok = Exchange.direct(chan, hospital_exchange, durable: true)
    :ok = Queue.bind(chan, response_queue, hospital_exchange, routing_key: tag)
    {:ok, _} = Basic.consume(chan, response_queue)

    {:ok, info_queue} = Queue.declare(chan)
    :ok = Exchange.topic(chan, admin_exchange, durable: true)
    :ok = Queue.bind(chan, info_queue.queue, admin_exchange, routing_key: info_key)
    {:ok, _} = Basic.consume(chan, info_queue.queue)
    state
  end

  ##############################################################################
  # Helpers
  ##############################################################################

  defp publish_and_log(channel, key, msg, config) do
    :ok = AMQP.Basic.publish(channel, config[:hospital_exchange], key, msg, reply_to: config[:tag])
    :ok = AMQP.Basic.publish(channel, config[:admin_exchange], config[:log_key], msg)
    :ok
  end
end
