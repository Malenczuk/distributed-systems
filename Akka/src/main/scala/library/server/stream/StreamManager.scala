package library.server.stream

import akka.actor.SupervisorStrategy.Restart
import akka.actor.{Actor, ActorLogging, OneForOneStrategy, Props}
import library.StreamRequest

import scala.concurrent.duration._

class StreamManager extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 10, withinTimeRange = 1 minute) {
      case _ => Restart
    }

  override def receive: Receive = {
    case request: StreamRequest =>
      context.actorOf(Props[StreamWorker]).tell(request, sender)
    case msg => log.info(s"Received [$msg]")
  }
}
