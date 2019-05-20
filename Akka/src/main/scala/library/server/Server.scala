package library.server

import akka.actor.SupervisorStrategy.Stop
import akka.actor.{Actor, ActorLogging, ActorSystem, OneForOneStrategy, Props}
import com.typesafe.config.{Config, ConfigFactory}
import library._
import library.server.database.DatabaseManager
import library.server.order.OrderManager
import library.server.stream.StreamManager

import scala.concurrent.duration._
import scala.io.StdIn

class Server extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 10, withinTimeRange = 1 minute) {
      case _ => Stop
    }
  private val databaseManager = context.actorOf(Props[DatabaseManager], name = "databaseManager")
  private val orderManager = context.actorOf(Props[OrderManager], name = "orderManager")
  private val streamManager = context.actorOf(Props[StreamManager], name = "streamManager")

  override def receive: Receive = {
    case request: SearchRequest =>
      databaseManager.tell(request, sender)
    case request: OrderRequest =>
      orderManager.tell(request, sender)
    case request: StreamRequest =>
      streamManager.tell(request, sender)
    case msg => log.info(s"Received [$msg]")
  }
}

object Server {
  def main(args: Array[String]): Unit = {
    val config: Config = ConfigFactory.load("server")
    val system = ActorSystem("server", config)
    system.actorOf(Props[Server], name = "server")

    Iterator.continually(StdIn.readLine()).takeWhile(_ != "q").foreach(_ => {})
    system.terminate()
  }
}
