package library.server.database

import akka.actor.SupervisorStrategy.Restart
import akka.actor.{Actor, ActorLogging, OneForOneStrategy, Props}
import library.SearchRequest

import scala.concurrent.duration._

class DatabaseManager extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 10, withinTimeRange = 1 minute) {
      case _ => Restart
    }

  override def receive: Receive = {
    case request: SearchRequest =>
      context.actorOf(Props[DatabaseWorker]).tell(request, sender)
    case msg => log.info(s"Received [$msg]")
  }
}
