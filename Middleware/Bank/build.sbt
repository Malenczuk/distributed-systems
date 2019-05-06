name := "bank.Bank"

version := "0.1"

scalaVersion := "2.12.8"
scalacOptions += "-Ypartial-unification"

PB.targets in Compile := Seq(
  scalapb.gen(grpc = true) -> (sourceManaged in Compile).value
)

libraryDependencies ++= Seq(
  "org.typelevel" %% "cats-core" % "1.6.0",
  "org.typelevel" %% "cats-effect" % "1.2.0",

  "com.thesamet.scalapb" %% "scalapb-runtime" % scalapb.compiler.Version.scalapbVersion % "protobuf",
  "io.grpc" % "grpc-netty" % scalapb.compiler.Version.grpcJavaVersion,
  "com.thesamet.scalapb" %% "scalapb-runtime-grpc" % scalapb.compiler.Version.scalapbVersion,

  "org.apache.thrift" % "libthrift" % "0.12.0",
  "com.twitter" %% "scrooge-core" % "19.4.0" exclude("com.twitter", "libthrift"),
  "com.twitter" %% "finagle-thrift" % "19.4.0" exclude("com.twitter", "libthrift"),
)
