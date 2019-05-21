package library.server.database

import akka.actor.{Actor, ActorLogging}
import library._
import net.liftweb.json._

import scala.io.Source

class SearchDatabaseWorker(val database: String) extends Actor with ActorLogging {
  implicit val formats: DefaultFormats = DefaultFormats

  override def postStop: Unit = {
    context.parent ! SearchResponse(None)
  }

  override def receive: Receive = {
    case SearchRequest(title) =>
      val source = Source.fromFile(database)
      val db: List[Book] = parse(try source.mkString finally source.close()).extract[List[Book]]
      sender ! SearchResponse(db.find(_.title == title))
    case msg => log.info(s"Received [$msg]")
  }
}
