package bank.services

import bank._
import com.twitter.util.Future

class PremiumManagerService(implicit val bankCurrencies: Seq[CurrencyCode], implicit val database: BankDatabase)
  extends PremiumManager.MethodPerEndpoint {


  override def askForLoan(auth: Auth, loanRequest: LoanRequest): Future[LoanResponse] = {
    database.getAuthBankClient(auth) match {
      case Right(client) =>
        database.loanRespond(client, loanRequest) match {
          case Right(response) => Future.value(response)
          case Left(error) => Future.exception(error)
        }
      case Left(error) => Future.exception(error)
    }
  }

  override def getBalance(auth: Auth): Future[Money] = {
    database.getAuthBankClient(auth) match {
      case Right(client) => Future.value(client.balance)
      case Left(error) => Future.exception(error)
    }
  }
}
