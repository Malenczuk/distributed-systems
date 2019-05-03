resolvers += Resolver.bintrayIvyRepo("twittercsl", "sbt-plugins")

addSbtPlugin("com.thesamet" % "sbt-protoc" % "0.99.20")
addSbtPlugin("com.twitter" % "scrooge-sbt-plugin" % "19.4.0")

libraryDependencies += "com.thesamet.scalapb" %% "compilerplugin" % "0.9.0-RC1"