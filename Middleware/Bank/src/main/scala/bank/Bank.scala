package bank

import java.net.{InetAddress, InetSocketAddress}

import bank.CurrencyCode._
import bank.services.{AccountCreatorService, PremiumManagerService, StandardManagerService}
import com.twitter.finagle.thrift.ThriftService
import com.twitter.finagle.{ListeningServer, Thrift}


class Bank() {
  val bankAddress = new InetSocketAddress(InetAddress.getLoopbackAddress, 9090)
  val accountAddress = new InetSocketAddress(InetAddress.getLoopbackAddress, 9091)
  val exchangePort: Int = 50051
  implicit val bankCurrencies: Seq[CurrencyCode] = Seq(Pln, Usd, Eur)
  implicit val bankDatabase: BankDatabase = new BankDatabase(bankCurrencies, exchangePort)
  private var accountServer: ListeningServer = _
  private var bankServer: ListeningServer = _

  def start(): Unit = {
    val serviceMap: Map[String, ThriftService] = Map(
      "Standard_Manager" -> new StandardManagerService(),
      "Premium_Manager" -> new PremiumManagerService()
    )

    accountServer = Thrift.server.withBufferedTransport().serveIface(accountAddress, new AccountCreatorService())
    bankServer = Thrift.server.withBufferedTransport().serveIfaces(bankAddress, serviceMap)

    sys.addShutdownHook {
      System.err.println("*** shutting down Thrift server since JVM is shutting down")
      stop()
      System.err.println("*** Thrift server shut down")
    }
  }

  private def stop(): Unit = {
    if (bankServer != null) {
      bankServer.close()
    }
    if (accountServer != null) {
      accountServer.close()
    }
  }
}

object Bank {
  def main(args: Array[String]): Unit = {
    new Bank().start()
    Thread.currentThread.join()
  }
}