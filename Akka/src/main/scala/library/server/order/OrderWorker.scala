package library.server.order


import java.nio.file.{Files, Paths, StandardOpenOption}

import akka.actor.{Actor, ActorLogging, ActorRef, ActorSystem}
import library._

import scala.concurrent.duration._
import scala.util.{Failure, Success}

class OrderWorker extends Actor with ActorLogging {
  implicit val actorSystem: ActorSystem = context.system

  import actorSystem.dispatcher

  override def receive: Receive = {
    case OrderRequest(title) =>
      val client = sender
      actorSystem.actorSelection("user/server/databaseManager").resolveOne(1 second).onComplete {
        case Success(databaseManager) =>
          databaseManager ! SearchRequest(title)
          context.become(receive(client))
        case Failure(_) =>
          client ! OrderResponse(false)
          context.stop(self)
      }
    case msg => log.info(s"Received [$msg]")
  }

  def receive(client: ActorRef): Receive = {
    case SearchResponse(None) =>
      client ! OrderResponse(false)
      context.stop(self)
    case SearchResponse(Some(book)) =>
      val orders = Paths.get("database/orders.txt")
      val order = (book.title + "\n").getBytes

      Files.write(orders, order, StandardOpenOption.APPEND, StandardOpenOption.SYNC)

      client ! OrderResponse(true)

      context.stop(self)

    case msg => log.info(s"Received [$msg]")
  }
}
