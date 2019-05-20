package library

sealed trait Request extends Serializable {
  def title: String
}

sealed trait DatabaseRequest extends Request {
  def database: String
}

final case class SearchRequest(title: String) extends Request

final case class SearchDatabaseRequest(title: String, database: String) extends DatabaseRequest

final case class OrderRequest(title: String) extends Request

final case class StreamRequest(title: String) extends Request
