package bank

import java.net.{InetAddress, InetSocketAddress}

import bank.CurrencyCode._
import bank.services.{AccountCreatorService, PremiumManagerService, StandardManagerService}
import com.twitter.finagle.thrift.ThriftService
import com.twitter.finagle.{ListeningServer, Thrift}


class Bank() {
  val bankPort: Int = 9090
  val exchangePort: Int = 50051
  implicit val bankCurrencies: Seq[CurrencyCode] = Seq(Pln, Usd, Eur)
  implicit val bankDatabase: BankDatabase = new BankDatabase(bankCurrencies, exchangePort)
  private var server: ListeningServer = _

  def start(): Unit = {
    val serviceMap: Map[String, ThriftService] = Map(
      "Account_Creator" -> new AccountCreatorService(),
      "Standard_Manager" -> new StandardManagerService(),
      "Premium_Manager" -> new PremiumManagerService()
    )

    val address = new InetSocketAddress(InetAddress.getLoopbackAddress, bankPort)
    server = Thrift.server.serveIfaces(address, serviceMap)

    sys.addShutdownHook {
      System.err.println("*** shutting down Thrift server since JVM is shutting down")
      stop()
      System.err.println("*** Thrift server shut down")
    }
  }

  private def stop(): Unit = {
    if (server != null) {
      server.close()
    }
  }
}

object Bank {
  def main(args: Array[String]): Unit = {
    new Bank().start()
    Thread.currentThread.join()
  }
}