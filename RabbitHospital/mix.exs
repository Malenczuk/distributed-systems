defmodule RabbitHospital.MixProject do
  use Mix.Project

  def project do
    [
      app: :rabbit_hospital,
      version: "0.1.0",
      elixir: "~> 1.8",
      start_permanent: Mix.env() == :prod,
      deps: deps()
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      applications: [:amqp],
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:amqp, "~> 1.1.1"},
      {:rabbit_common, "3.7.11"},
      {:apex, "~>1.2.1"},
      {:uuid, "~> 1.1.8"}
    ]
  end
end
