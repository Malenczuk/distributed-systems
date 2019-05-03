-module(client_app).

-behaviour(application).
-include("gen-erl/bank_types.hrl").

%% API
-export([create_account/0,
  create_account/5,
  get_balance/0,
  get_balance/2,
  get_loan_cost/0,
  get_loan_cost/3,
  repl/0]).

%% Application callbacks
-export([start/2,
  stop/1]).

%%%===================================================================
%%% API
%%%===================================================================

repl() ->
  case io:fread("Select an option
   1: Create Account
   2: Get Account Balance
   3: Get Loan Cost [P]
   > ", "~d") of
    {ok, [1]} -> create_account();
    {ok, [2]} -> get_balance();
    {ok, [3]} -> get_loan_cost();
    {_, Input} -> io:format("Bad Input (~p)~n", [Input])
  end,
  repl().

create_account() ->
  case io:fread("Enter space separated Name, Surname, Pesel, Income and Inital Balance\n", "~s ~s ~d ~d ~d") of
    {ok, [Name, Surname, Uid, Income, InitialBalance]} ->
      io:format("~p~n", [create_account(Name, Surname, Uid, Income, InitialBalance)]);
    {_, Reason} -> io:format("Bad Input (~p)~n", [Reason])
  end.

create_account(Forename, Surname, Uid, Income, InitialBalance) ->
  UserData = #'UserData'{name = Forename, surname = Surname, uid = #'UID'{value = Uid}, income = #'Money'{value = Income}},
  Init = #'Money'{value = InitialBalance},
  gen_server:call(client_serv, {create_account, [UserData, Init]}).

get_balance() ->
  case io:fread("Enter space separated GUID and Account Type [ STANDARD | PREMIUM ]\n", "~s ~a") of
    {ok, [Guid, Type]} when (Type == 'PREMIUM') or (Type == 'STANDARD') ->
      io:format("~p~n", [get_balance(Guid, Type)]);
    {_, Reason} -> io:format("Bad Input (~p)~n", [Reason])
  end.

get_balance(Guid, 'PREMIUM') ->
  GUID = #'GUID'{value = Guid},
  gen_server:call(client_serv, {get_balance_premium, [GUID]});

get_balance(Guid, 'STANDARD') ->
  GUID = #'GUID'{value = Guid},
  gen_server:call(client_serv, {get_balance_standard, [GUID]}).

get_loan_cost() ->
  case io:fread("Enter space separated GUID, Loan Amount and Currency Code\n", "~s ~d ~a") of
    {ok, [Guid, Amount, Currency]} ->
      io:format("~p~n", [get_loan_cost(Guid, Amount, Currency)]);
    {_, Reason} -> io:format("Bad Input (~p)~n", [Reason])
  end.

get_loan_cost(Guid, Amount, Currency) ->
  case lists:keyfind(Currency, 1, bank_types:enum_info('CurrencyCode')) of
    {_, Code} ->
      Request = #'LoanRequest'{guid = #'GUID'{value = Guid}, amount = #'Money'{value = Amount}, currency = Code},
      gen_server:call(client_serv, {get_loan_cost, [Request]});
    false -> {error, wrong_currency_code}
  end.

%% ===================================================================
%% Application callbacks
%% ===================================================================

start(_StartType, _StartArgs) ->
  spawn(?MODULE, repl, []),
  client_sup:start_link().


stop(_State) ->
  ok.
