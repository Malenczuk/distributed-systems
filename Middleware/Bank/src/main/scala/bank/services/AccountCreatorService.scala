package bank.services

import java.util.UUID

import bank.AccountType.{Premium, Standard}
import bank.{BankDatabase, _}
import com.twitter.util.Future

class AccountCreatorService(implicit private val database: BankDatabase) extends AccountCreator.MethodPerEndpoint {

  override def registerClient(data: UserData, initialBalance: Money): Future[BankClient] = {
    val clientType = if (database.isPremium(data.income)) Premium else Standard
    val guid = Guid(UUID.randomUUID.toString)
    val newClient = BankClient(data, clientType, guid, initialBalance)

    database.addClient(newClient) match {
      case Right(client) => Future.value(client)
      case Left(error) => Future.exception(error)
    }
  }
}
