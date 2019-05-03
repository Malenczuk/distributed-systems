package bank.services

import bank.{BankDatabase, _}
import com.twitter.util.Future

class StandardManagerService(implicit val database: BankDatabase) extends StandardManager.MethodPerEndpoint {
  override def getBalance(auth: Auth): Future[Money] = {
    database.getAuthBankClient(auth) match {
      case Right(client) => Future.value(client.balance)
      case Left(error) => Future.exception(error)
    }
  }
}
