package currency.exchange.services

import currency.exchange.{CurrencyRate, ExchangeGrpc, ExchangeRateRequest, ExchangeRateResponse, StockMarket}
import io.grpc.stub.StreamObserver

import scala.concurrent.Future

class ExchangeService extends ExchangeGrpc.Exchange {
  override def currentRates(request: ExchangeRateRequest): Future[ExchangeRateResponse] = {
    Future.successful(
      ExchangeRateResponse.of(StockMarket.getAllRates.filter(r => request.currencies.contains(r.code)))
    )
  }

  override def streamRates(request: ExchangeRateRequest, responseObserver: StreamObserver[CurrencyRate]): Unit = {
    val queue = StockMarket.subscribe
    val requestedCodes = request.currencies
    while (true) {
      val newRate = queue.take
      if (requestedCodes.contains(newRate.code))
        responseObserver.onNext(newRate)
    }
    responseObserver.onCompleted()
  }
}
