package library.server.database

import akka.actor.SupervisorStrategy.Stop
import akka.actor.{Actor, ActorLogging, ActorRef, OneForOneStrategy, Props}
import library._
import monix.execution.atomic.AtomicInt

import scala.concurrent.duration._

class DatabaseWorker(val databases: Seq[String]) extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 5, withinTimeRange = 10 seconds) {
      case _ => Stop
    }
  val counter: AtomicInt = AtomicInt(databases.size)


  override def postStop(): Unit = {
    context.children.foreach(context.stop)
  }

  override def receive: Receive = {
    case request: SearchRequest =>
      context.become(receive(sender))
      databases.foreach(db => context.actorOf(Props(new SearchDatabaseWorker(db))) ! request)
    case msg => log.info(s"Received [$msg]")
  }

  def receive(client: ActorRef): Receive = {
    case SearchResponse(None) if counter.get > 1 =>
      counter.decrement(1)
    case response: SearchResponse =>
      client ! response
      context.stop(self)
    case msg => log.info(s"Received [$msg]")
  }
}
