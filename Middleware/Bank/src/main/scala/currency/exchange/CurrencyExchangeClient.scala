package currency.exchange

import java.util.concurrent.TimeUnit
import java.util.logging.Logger

import currency.exchange.ExchangeGrpc.ExchangeBlockingStub
import io.grpc.{ManagedChannel, ManagedChannelBuilder}

import scala.collection.mutable

class CurrencyExchangeClient private(private val channel: ManagedChannel,
                                     private val blockingStub: ExchangeBlockingStub,
                                     private val currencies: Seq[CurrencyCode]) {

  val rates: mutable.Map[CurrencyCode, Double] = mutable.Map[CurrencyCode, Double]()

  def start(): Unit = {
    blockingStub.currentRates(new ExchangeRateRequest(currencies)).rates.foreach(rate => rates.put(rate.code, rate.rate))

    new Thread(() => {
      blockingStub.streamRates(new ExchangeRateRequest(currencies)).foreach(rate =>
        rates.synchronized {
          CurrencyExchangeClient.logger.info(s"Updating ${rate.code} [ ${rates(rate.code)} -> ${rate.rate} ]")
          rates.put(rate.code, rate.rate)
        })
    }).start()

    sys.addShutdownHook {
      System.err.println("*** shutting down gRPC client since JVM is shutting down")
      stop()
      System.err.println("*** client shut down")
    }
  }

  private def stop(): Unit = {
    if (channel != null) {
      channel.shutdown().awaitTermination(1, TimeUnit.SECONDS)
    }
  }
}

object CurrencyExchangeClient {
  private val logger = Logger.getLogger(classOf[CurrencyExchangeClient].getName)


  def apply(host: String, port: Int, currencyCodes: Seq[CurrencyCode]): CurrencyExchangeClient = {
    val channel = ManagedChannelBuilder.forAddress(host, port).usePlaintext().build()
    val blockingStub = ExchangeGrpc.blockingStub(channel)
    new CurrencyExchangeClient(channel, blockingStub, currencyCodes)
  }
}
