defmodule Technician do
  @moduledoc false
  use AMQP
  @behaviour Worker

  require Logger
  alias Message


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
      hospital_exchange: "hospital",
      admin_exchange: "admin",
      info_key: "info",
      log_key: "log",
      spec: ["knee", "elbow"]
    ]
  end

  def handle_message(%Message{attributes: attributes, state: %{chan: chan, config: config, module: module}} = message) do
    name = Atom.to_string(config[:name])
    hospital_exchange = config[:hospital_exchange]
    admin_exchange = config[:admin_exchange]
    case attributes.exchange do
      ^hospital_exchange ->
        Logger.info("[#{module}][#{name}] Received #{attributes.routing_key} request: #{message.payload}")
        :rand.uniform(4) |> Kernel.*(1000) |> :timer.sleep
        response = "#{message.payload} #{attributes.routing_key} done"
        Logger.info("[#{module}][#{name}] Diagnose: #{response}")
        ack(message)
        publish_and_log(chan, attributes.reply_to, response, config)
      ^admin_exchange ->
        Logger.info("[#{module}][#{name}] Received info: #{message.payload}")
        ack(message)
      _ ->
        Logger.info("[#{module}][#{name}] Received message: #{inspect(message)}")
        ack(message)
    end

  rescue
    exception ->
      Logger.error(Exception.format(:error, exception, System.stacktrace()))
      reject(message, false)
  end

  def handle_input(_) do
    :ok
  end

  def worker_tag() do
    "#{__MODULE__}_#{UUID.uuid1()}"
  end

  def setup_queues(%{chan: chan, config: config} = state) do
    hospital_exchange = config[:hospital_exchange]
    admin_exchange = config[:admin_exchange]
    info_key = config[:info_key]
    prefetch_count = String.to_integer(config[:prefetch_count])
    specializations = config[:spec]

    :ok = Basic.qos(chan, prefetch_count: prefetch_count)
    :ok = Exchange.direct(chan, hospital_exchange, durable: true)

    Enum.each specializations, fn spec ->
      spec_queue = "#{spec}_queue"
      {:ok, _} = Queue.declare(chan, spec_queue)
      :ok = Queue.bind(chan, spec_queue, hospital_exchange, routing_key: spec)
      {:ok, _} = Basic.consume(chan, spec_queue)
    end

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
    :ok = AMQP.Basic.publish(
      channel,
      config[:hospital_exchange],
      key,
      msg,
      reply_to: config[:tag]
    )
    :ok = AMQP.Basic.publish(channel, config[:admin_exchange], config[:log_key], msg)
    :ok
  end
end