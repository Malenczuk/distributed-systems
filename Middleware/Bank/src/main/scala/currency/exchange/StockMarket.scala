package currency.exchange

import java.util.concurrent.BlockingQueue
import java.util.logging.Logger

import scala.collection.mutable
import scala.collection.mutable.ListBuffer
import scala.util.Random

private class StockMarket(currencyCode: CurrencyCode) extends Thread {
  override def run(): Unit = {
    if (currencyCode == CurrencyCode.PLN) {
      return
    }
    while (true) {
      Thread.sleep(2500 + StockMarket.random.nextInt(5000))

      val delta = StockMarket.random.nextDouble() / 10
      val sign = if (StockMarket.random.nextBoolean()) 1 else -1
      val newRate = StockMarket.rates.synchronized {
        val oldValue = StockMarket.rates(currencyCode)
        val newValue = math.max(0.01, oldValue + sign * oldValue * delta)
        StockMarket.rates.put(currencyCode, newValue)
        CurrencyRate(currencyCode, newValue)
      }
      StockMarket.logger.info(s"Updating ${newRate.code} to ${newRate.rate}")
      StockMarket.queues.synchronized {
        StockMarket.queues.foreach(q => q.put(newRate))
      }

    }
  }
}

object StockMarket {
  private val logger = Logger.getLogger(classOf[StockMarket].getName)
  private val random = new Random()
  private val rates = mutable.Map[CurrencyCode, Double](
    CurrencyCode.PLN -> 1.00,
    CurrencyCode.EUR -> 0.23,
    CurrencyCode.USD -> 0.26,
    CurrencyCode.GBP -> 0.20
  )
  private val queues = ListBuffer[BlockingQueue[CurrencyRate]]()

  rates.map(entry => {
    val market = new StockMarket(entry._1)
    market.start()
    market
  })

  import java.util.concurrent.{BlockingQueue, LinkedBlockingQueue}

  def subscribe: BlockingQueue[CurrencyRate] = {
    val result = new LinkedBlockingQueue[CurrencyRate]
    queues synchronized {
      queues += result
    }
    result
  }

  def getAllRates: List[CurrencyRate] = rates.synchronized {
    rates.map(entry => CurrencyRate(entry._1, entry._2)).toList
  }

}
