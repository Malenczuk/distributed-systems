package library.server.database

import java.io.FileNotFoundException

import akka.actor.SupervisorStrategy.{Restart, Stop}
import akka.actor.{Actor, ActorLogging, ActorRef, OneForOneStrategy, Props}
import library._

import scala.concurrent.duration._

class DatabaseWorker extends Actor with ActorLogging {
  override val supervisorStrategy: OneForOneStrategy =
    OneForOneStrategy(maxNrOfRetries = 10, withinTimeRange = 1 minute) {
      case _: FileNotFoundException => Stop
      case _ => Restart
    }

  val databases: List[String] = List("database/db1.json", "database/db2.json")

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
    case SearchResponse(None) if context.children.size > 1 =>
      context.stop(sender)
    case response: SearchResponse =>
      client ! response
      context.stop(self)
    case msg => log.info(s"Received [$msg]")
  }
}
