package library.server.order

import java.nio.file.NoSuchFileException

import akka.actor.SupervisorStrategy.{Escalate, Restart}
import akka.actor.{Actor, ActorLogging, OneForOneStrategy, Props}
import library.OrderRequest

import scala.concurrent.duration._

class OrderManager extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 10, withinTimeRange = 1 minute) {
      case _: NoSuchFileException => Escalate
      case _ => Restart
    }

  override def receive: Receive = {
    case request: OrderRequest =>
      context.actorOf(Props[OrderWorker]).tell(request, sender)
    case msg => log.info(s"Received [$msg]")
  }
}
