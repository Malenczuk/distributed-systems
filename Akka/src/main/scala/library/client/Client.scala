package library.client

import akka.actor.ActorRef.noSender
import akka.actor.{Actor, ActorLogging, ActorSystem, Props}
import com.typesafe.config.{Config, ConfigFactory}
import library._

import scala.io.StdIn

class Client(server: String) extends Actor with ActorLogging {
  private val searchCmd = "search (.+)".r
  private val orderCmd = "order (.+)".r
  private val streamCmd = "stream (.+)".r

  override def receive: Receive = {
    case cmd: String => cmd match {
      case searchCmd(instr) =>
        context.actorSelection(server) ! SearchRequest(instr)
      case orderCmd(instr) =>
        context.actorSelection(server) ! OrderRequest(instr)
      case streamCmd(instr) =>
        context.actorSelection(server) ! StreamRequest(instr)
      case instr =>
        println(s"Bad command: '$instr'")
    }
    case SearchResponse(None) =>
      println(s"Search: Failed")
    case SearchResponse(Some(book)) =>
      println(s"Search: $book")
    case OrderResponse(status) =>
      println(s"Order: ${if (status) "Succeed" else "Failed"}")
    case StreamResponse(stream) =>
      println(stream.utf8String)
    case msg => log.info(s"Received [$msg]")
  }
}

object Client {
  private val SERVER_PATH = "akka.tcp://server@127.0.0.1:2137/user/server"


  def main(args: Array[String]): Unit = {
    val config: Config = ConfigFactory.load("client")
    val system = ActorSystem("client", config)
    val client = system.actorOf(Props(new Client(SERVER_PATH)), name = "client")
    println("Started. Commands: 'search [title]', 'order [title]', 'stream [title]'")

    Iterator.continually(StdIn.readLine()).takeWhile(_ != "q").foreach(client.tell(_, noSender))
    system.terminate()
  }
}
