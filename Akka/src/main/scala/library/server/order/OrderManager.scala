package library.server.order


import akka.actor.SupervisorStrategy.Stop
import akka.actor.{Actor, ActorLogging, OneForOneStrategy, Props}
import library.OrderRequest

import scala.concurrent.duration._

class OrderManager extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 5, withinTimeRange = 10 seconds) {
      case _ => Stop
    }

  override def receive: Receive = {
    case request: OrderRequest =>
      context.actorOf(Props[OrderWorker]).tell(request, sender)
    case msg => log.info(s"Received [$msg]")
  }
}
