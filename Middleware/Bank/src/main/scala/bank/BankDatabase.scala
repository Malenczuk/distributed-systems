package bank

import java.security.MessageDigest

import currency.CurrencyAdapter._
import currency.exchange.CurrencyExchangeClient

import scala.collection.mutable

class BankDatabase(currencyCodes: Seq[CurrencyCode], exchangePort: Int) {
  private val premiumThreshold = Money(5000 * 100)
  private val loanAmountInterest = 0.15
  private val loanPeriodInterest = 0.01
  private val exchangeClient = CurrencyExchangeClient("localhost", exchangePort, currencyCodes)
  private val clients: mutable.Map[Uid, BankClient] = mutable.Map()
  exchangeClient.start()

  def addClient(client: BankClient): Either[InvalidUidError, BankClient] = {
    if (clients.isDefinedAt(client.data.uid)) {
      Left(InvalidUidError(client.data.uid, "Client already exists"))
    } else {
      clients.put(client.data.uid, client)
      Right(client)
    }
  }

  def getAuthBankClient(auth: Auth): Either[InvalidAuthError, BankClient] = {
    getBankClient(auth.uid) match {
      case Right(client) if md5(client.guid.value) == auth.guid.value => Right(client)
      case Right(client) =>
        println(md5(client.guid.value))
        Left(InvalidAuthError(auth, "Incorrect GUID"))
      case Left(error) => Left(InvalidAuthError(auth, error.reason))
    }
  }

  def getBankClient(uid: Uid): Either[InvalidUidError, BankClient] = {
    Either.cond(
      clients.isDefinedAt(uid),
      clients(uid),
      InvalidUidError(uid, "Client does not exists")
    )
  }

  def loanRespond(client: BankClient, request: LoanRequest): Either[InvalidRequestError, LoanResponse] = {
    if (isPremium(client.data.income)) {
      (getRate(request.currency), calculateWithInterest(request.amount, request.period)) match {
        case (Some(rate), Some(cost)) =>
          val baseCost = Some(Money((cost / rate).round))
          val targetCost = Some(Money(cost.round))
          Right(LoanResponse(accepted = true, baseCost = baseCost, targetCost = targetCost))
        case (None, _) => Left(InvalidRequestError(request, "Currency not serviced by the bank"))
        case _ => Left(InvalidRequestError(request, "Invalid Requested amount or period"))
      }
    } else {
      Left(InvalidRequestError(request, "Not a Premium client"))
    }
  }

  def md5(s: String): String = {
    MessageDigest.getInstance("MD5").digest(s.getBytes).map("%02x".format(_)).mkString
  }

  def getRate(code: CurrencyCode): Option[Double] = exchangeClient.rates.get(code)

  def calculateWithInterest(amount: Money, period: Period): Option[Double] = {
    if (period.months < 0 || amount.value < 0) None
    else Some((1 + loanAmountInterest + loanPeriodInterest * period.months) * amount.value)
  }

  def isPremium(salary: Money): Boolean = salary.value >= premiumThreshold.value
}
