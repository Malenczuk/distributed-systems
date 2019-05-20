package library.server.stream

import java.nio.file.Paths

import akka.actor.{Actor, ActorLogging, ActorRef, ActorSystem}
import akka.stream.scaladsl.{FileIO, Flow, Framing, Sink, Source}
import akka.stream.{ActorMaterializer, IOResult, ThrottleMode}
import akka.util.ByteString
import akka.{Done, NotUsed}
import library.{SearchRequest, SearchResponse, StreamRequest, StreamResponse}

import scala.concurrent.Future
import scala.concurrent.duration._
import scala.util.{Failure, Success}


class StreamWorker extends Actor with ActorLogging {
  implicit val actorSystem: ActorSystem = context.system
  implicit val flowMaterializer: ActorMaterializer = ActorMaterializer()

  import actorSystem.dispatcher

  override def receive: Receive = {
    case StreamRequest(title) =>
      val client = sender
      actorSystem.actorSelection("user/server/databaseManager").resolveOne(1 second).onComplete {
        case Success(databaseManager) =>
          databaseManager ! SearchRequest(title)
          context.become(receive(client))
        case Failure(_) =>
          client ! StreamResponse(ByteString("Database Failure"))
          context.stop(self)
      }
    case msg => log.info(s"Received [$msg]")
  }

  def receive(client: ActorRef): Receive = {
    case SearchResponse(None) =>
      client ! StreamResponse(ByteString("Book Not Found"))
      context.stop(self)
    case SearchResponse(Some(book)) =>

      val source: Option[Source[ByteString, Future[IOResult]]] = {
        Paths.get(s"database/books/${book.file}") match {
          case path if path.toFile.isFile => Some(FileIO.fromPath(path))
          case _ => None
        }
      }

      val flow: Flow[ByteString, ByteString, NotUsed] =
        Framing.delimiter(ByteString(System.lineSeparator()), maximumFrameLength = 512, allowTruncation = true)
          .throttle(1, 1 second, 1, ThrottleMode.shaping)

      val sink: Sink[ByteString, Future[Done]] = Sink.foreach(client ! StreamResponse(_))

      source match {
        case Some(s) => s.via(flow).runWith(sink).andThen({ case _ => context.stop(self) })
        case None =>
          client ! StreamResponse(ByteString("Book File Not Found"))
          context.stop(self)
      }
    case msg => log.info(s"Received [$msg]")
  }
}
