package currency

object CurrencyAdapter {
  implicit def adapt(code: bank.CurrencyCode): exchange.CurrencyCode = {
    exchange.CurrencyCode.fromValue(code.value)
  }

  implicit def adapt(codes: Seq[bank.CurrencyCode]): Seq[exchange.CurrencyCode] = {
    codes.map(code => exchange.CurrencyCode.fromValue(code.value))
  }
}
