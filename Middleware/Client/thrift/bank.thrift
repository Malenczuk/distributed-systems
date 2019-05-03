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

struct Period {
    1: i64 months
}

struct UID {
    1: i64 value
}

struct GUID {
    1: string value
}

struct Auth {
    1: UID uid,
    2: GUID guid
}

struct UserData {
    1: string name,
    2: string surname,
    3: UID uid,
    4: Money income
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
    3: Period period
}

struct LoanResponse {
    1: required bool accepted,
    2: optional Money baseCost,
    3: optional Money targetCost
}

exception InvalidAuthError {
    1: Auth auth,
    2: string reason
}
exception InvalidUidError {
    1: UID uid,
    2: string reason
}
exception InvalidRequestError {
    1: LoanRequest request,
    2: string reason
}

service AccountCreator {
    BankClient registerClient(1:UserData data, 2:Money initialBalance)
        throws (1: InvalidUidError ue),
}

service StandardManager {
    Money getBalance(1:Auth auth) throws (1: InvalidAuthError ae),
}

service PremiumManager extends StandardManager {
    LoanResponse askForLoan(1: Auth auth, 2:LoanRequest loanRequest)
        throws (1: InvalidAuthError ae, 2: InvalidRequestError re),
}
