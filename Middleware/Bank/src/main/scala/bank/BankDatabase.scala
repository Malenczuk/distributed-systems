package bank

import currency.CurrencyAdapter._
import currency.exchange.CurrencyExchangeClient

import scala.collection.mutable

class BankDatabase(currencyCodes: Seq[CurrencyCode], exchangePort: Int) {
  private val premiumThreshold = Money(5000 * 100)
  private val loanInterest = 0.15
  private val exchangeClient = CurrencyExchangeClient("localhost", exchangePort, currencyCodes)
  private val clients: mutable.Map[Guid, BankClient] = mutable.Map()
  exchangeClient.start()

  def addClient(client: BankClient): Either[InvalidGuid, BankClient] = {
    Either.cond(
      !clients.isDefinedAt(client.guid),
      {
        clients.put(client.guid, client)
        client
      },
      InvalidGuid(client.guid, "Client already exists")
    )
  }

  def getBankClient(guid: Guid): Either[InvalidGuid, BankClient] = {
    Either.cond(
      clients.isDefinedAt(guid),
      clients(guid),
      InvalidGuid(guid, "Client does not exists")
    )
  }

  def getRate(code: CurrencyCode): Double = exchangeClient.rates(code)

  def loanRespond(request: LoanRequest)(implicit database: BankDatabase): Either[InvalidGuid, LoanResponse] = {
    database.getBankClient(request.guid) match {
      case Right(client) if isPremium(client.data.income) =>
        val withInterest = (1 + loanInterest) * request.amount.value
        val targetWithInterest = withInterest * database.getRate(request.currency)
        val baseCost = Some(Money(withInterest.round))
        val targetCost = Some(Money(targetWithInterest.round))
        Right(LoanResponse(accepted = true, baseCost = baseCost, targetCost = targetCost))
      case Right(_) => Right(LoanResponse(accepted = false))
      case Left(error) => Left(error)
    }
  }

  def isPremium(salary: Money): Boolean = salary.value >= premiumThreshold.value
}
