package library.server.database

import akka.actor.SupervisorStrategy.Stop
import akka.actor.{Actor, ActorLogging, OneForOneStrategy, Props}
import library.SearchRequest

import scala.concurrent.duration._

class DatabaseManager extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 5, withinTimeRange = 10 seconds) {
      case _ => Stop
    }

  val databases: List[String] = List("database/db4.json", "database/db3.json")

  override def receive: Receive = {
    case request: SearchRequest =>
      context.actorOf(Props(new DatabaseWorker(databases))).tell(request, sender)
    case msg => log.info(s"Received [$msg]")
  }
}
