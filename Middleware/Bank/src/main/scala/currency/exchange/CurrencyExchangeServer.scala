package currency.exchange

import java.util.logging.Logger

import currency.exchange.services.ExchangeService
import io.grpc.{Server, ServerBuilder}

import scala.concurrent.ExecutionContext


class CurrencyExchangeServer(executionContext: ExecutionContext) {
  private var server: Server = _

  private def start() = {
    server = ServerBuilder.forPort(CurrencyExchangeServer.port)
      .addService(ExchangeGrpc.bindService(new ExchangeService, executionContext)).build.start
    CurrencyExchangeServer.logger.info("Server started, listening on " + CurrencyExchangeServer.port)
    sys.addShutdownHook {
      System.err.println("*** shutting down gRPC server since JVM is shutting down")
      stop()
      System.err.println("*** gRPC server shut down")
    }
  }

  private def stop(): Unit = {
    if (server != null) {
      server.shutdown()
    }
  }

  private def blockUntilShutdown(): Unit = {
    if (server != null) {
      server.awaitTermination()
    }
  }
}

object CurrencyExchangeServer {
  private val logger = Logger.getLogger(classOf[CurrencyExchangeServer].getName)
  private val port = 50051

  def main(args: Array[String]): Unit = {
    StockMarket
    val server = new CurrencyExchangeServer(ExecutionContext.global)
    server.start()
    server.blockUntilShutdown()
  }
}