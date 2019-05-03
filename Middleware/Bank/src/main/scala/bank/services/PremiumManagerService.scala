package bank.services

import bank._
import com.twitter.util.Future

class PremiumManagerService(implicit val bankCurrencies: Seq[CurrencyCode], implicit val database: BankDatabase)
  extends PremiumManager.MethodPerEndpoint {


  override def askForLoan(loanRequest: LoanRequest): Future[LoanResponse] = {
    if (bankCurrencies.contains(loanRequest.currency))
      database.loanRespond(loanRequest) match {
        case Right(response) => Future.value(response)
        case Left(error) => Future.exception(error)
      }
    else
      Future.value(LoanResponse(accepted = false))
  }

  override def getBalance(guid: Guid): Future[Money] = {
    database.getBankClient(guid) match {
      case Right(client) => Future.value(client.balance)
      case Left(error) => Future.exception(error)
    }
  }
}
