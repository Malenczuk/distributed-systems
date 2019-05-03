namespace java bank

enum CurrencyCode {
    PLN = 0;
    USD = 1;
    EUR = 2;
    GBP = 3;
}

enum AccountType {
    STANDARD = 0,
    PREMIUM = 1
}

struct Money {
    1: i64 value
}

struct UID {
    1: i64 value
}

struct GUID {
    1: string value
}

struct UserData {
    1: string name,
    2: string surname,
    3: UID uid,
    4: Money income,
}

struct BankClient {
    1: UserData data,
    2: AccountType type,
    3: GUID guid,
    4: Money balance
}

struct LoanRequest {
    1: Money amount,
    2: CurrencyCode currency,
    3: GUID guid
}

struct LoanResponse {
    1: required bool accepted,
    2: optional Money baseCost,
    3: optional Money targetCost
}
exception InvalidGuid {
    1: GUID guid,
    2: string reason
}

service AccountCreator {
    BankClient registerClient(1:UserData data, 2:Money initialBalance) throws (1: InvalidGuid ex),
}

service StandardManager {
    Money getBalance(1:GUID guid) throws (1: InvalidGuid ex),
}

service PremiumManager extends StandardManager {
    LoanResponse askForLoan(1:LoanRequest loanRequest) throws (1: InvalidGuid ex),
}
