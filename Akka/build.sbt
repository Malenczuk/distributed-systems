name := "akka-library"

version := "1.0"

scalaVersion := "2.12.8"

lazy val akkaVersion = "2.5.22"
lazy val akkaHttpVersion = "10.1.8"
lazy val liftVersion = "3.3.0"

libraryDependencies ++= Seq(
  "com.typesafe.akka" %% "akka-actor" % akkaVersion,
  "com.typesafe.akka" %% "akka-remote" % akkaVersion,
  "com.typesafe.akka" %% "akka-testkit" % akkaVersion % Test,
  "com.typesafe.akka" %% "akka-stream" % akkaVersion,
  "com.typesafe.akka" %% "akka-stream-testkit" % akkaVersion % Test,
  "org.scalatest" %% "scalatest" % "3.0.5" % Test,

  "net.liftweb" %% "lift-json" % liftVersion,
)

scalacOptions += "-Ypartial-unification"