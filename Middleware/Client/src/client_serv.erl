%%%-------------------------------------------------------------------
%%% @author malen
%%% @copyright (C) 2019, <COMPANY>
%%% @doc
%%%
%%% @end
%%% Created : 02. May 2019 17:42
%%%-------------------------------------------------------------------
-module(client_serv).
-author("malen").

-behaviour(gen_server).
-include("gen-erl/bank_types.hrl").

%% API
-export([start_link/0]).

%% gen_server callbacks
-export([init/1,
  handle_call/3,
  handle_cast/2,
  handle_info/2,
  terminate/2,
  code_change/3]).

-define(SERVER, ?MODULE).

-record(state, {account_creator, standard_manager, premium_manager}).

%%%===================================================================
%%% API
%%%===================================================================

%%--------------------------------------------------------------------
%% @doc
%% Starts the server
%%
%% @end
%%--------------------------------------------------------------------
-spec(start_link() ->
  {ok, Pid :: pid()} | ignore | {error, Reason :: term()}).
start_link() ->
  gen_server:start_link({local, ?SERVER}, ?MODULE, [], []).

%%%===================================================================
%%% gen_server callbacks
%%%===================================================================

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Initializes the server
%%
%% @spec init(Args) -> {ok, State} |
%%                     {ok, State, Timeout} |
%%                     ignore |
%%                     {stop, Reason}
%% @end
%%--------------------------------------------------------------------
-spec(init(Args :: term()) ->
  {ok, State :: #state{}} | {ok, State :: #state{}, timeout() | hibernate} |
  {stop, Reason :: term()} | ignore).
init([]) ->
  Port = 9090,
  Services = [
    {"Account_Creator", account_creator_thrift},
    {"Standard_Manager", standard_manager_thrift},
    {"Premium_Manager", premium_manager_thrift}
  ],

  {ok, [{"Account_Creator", Account_Creator},
    {"Standard_Manager", Standard_Manager},
    {"Premium_Manager", Premium_Manager}]} = thrift_client_util:new_multiplexed("127.0.0.1", Port, Services, [{framed, true}]),

  {ok, #state{account_creator = Account_Creator, standard_manager = Standard_Manager, premium_manager = Premium_Manager}}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Handling call messages
%%
%% @end
%%--------------------------------------------------------------------
-spec(handle_call(Request :: term(), From :: {pid(), Tag :: term()},
    State :: #state{}) ->
  {reply, Reply :: term(), NewState :: #state{}} |
  {reply, Reply :: term(), NewState :: #state{}, timeout() | hibernate} |
  {noreply, NewState :: #state{}} |
  {noreply, NewState :: #state{}, timeout() | hibernate} |
  {stop, Reason :: term(), Reply :: term(), NewState :: #state{}} |
  {stop, Reason :: term(), NewState :: #state{}}).
handle_call({create_account, Args}, _From, State) ->
  try thrift_client:call(State#state.account_creator, 'registerClient', Args) of
    {_, {ok, Reply}} -> {reply, format_reply(Reply), State};
    {_, {error, Error}} -> {reply, core_parse:format_error(Error), State}
  catch
    throw:{_, {_, Ex}} -> {reply, format_exception(Ex), State}
  end;

handle_call({get_balance_standard, Args}, _From, State) ->
  try thrift_client:call(State#state.standard_manager, 'getBalance', Args) of
    {_, {ok, Reply}} -> {reply, format_reply(Reply), State};
    {_, {error, Error}} -> {reply, core_parse:format_error(Error), State}
  catch
    throw:{_, {_, Ex}} -> {reply, format_exception(Ex), State}
  end;

handle_call({get_balance_premium, Args}, _From, State) ->
  try thrift_client:call(State#state.premium_manager, 'getBalance', Args) of
    {_, {ok, Reply}} -> {reply, format_reply(Reply), State};
    {_, {error, Error}} -> {reply, core_parse:format_error(Error), State}
  catch
    throw:{_, {_, Ex}} -> {reply, format_exception(Ex), State}
  end;

handle_call({get_loan_cost, Args}, _From, State) ->
  try thrift_client:call(State#state.premium_manager, 'askForLoan', Args) of
    {_, {ok, Reply}} -> {reply, format_reply(Reply), State};
    {_, {error, Error}} -> {reply, core_parse:format_error(Error), State}
  catch
    throw:{_, {_, Ex}} -> {reply, format_exception(Ex), State}
  end;

handle_call(_Request, _From, State) ->
  {reply, ok, State}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Handling cast messages
%%
%% @end
%%--------------------------------------------------------------------
-spec(handle_cast(Request :: term(), State :: #state{}) ->
  {noreply, NewState :: #state{}} |
  {noreply, NewState :: #state{}, timeout() | hibernate} |
  {stop, Reason :: term(), NewState :: #state{}}).
handle_cast(_Request, State) ->
  {noreply, State}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Handling all non call/cast messages
%%
%% @spec handle_info(Info, State) -> {noreply, State} |
%%                                   {noreply, State, Timeout} |
%%                                   {stop, Reason, State}
%% @end
%%--------------------------------------------------------------------
-spec(handle_info(Info :: timeout() | term(), State :: #state{}) ->
  {noreply, NewState :: #state{}} |
  {noreply, NewState :: #state{}, timeout() | hibernate} |
  {stop, Reason :: term(), NewState :: #state{}}).
handle_info(_Info, State) ->
  {noreply, State}.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% This function is called by a gen_server when it is about to
%% terminate. It should be the opposite of Module:init/1 and do any
%% necessary cleaning up. When it returns, the gen_server terminates
%% with Reason. The return value is ignored.
%%
%% @spec terminate(Reason, State) -> void()
%% @end
%%--------------------------------------------------------------------
-spec(terminate(Reason :: (normal | shutdown | {shutdown, term()} | term()),
    State :: #state{}) -> term()).
terminate(_Reason, _State) ->
  ok.

%%--------------------------------------------------------------------
%% @private
%% @doc
%% Convert process state when code is changed
%%
%% @spec code_change(OldVsn, State, Extra) -> {ok, NewState}
%% @end
%%--------------------------------------------------------------------
-spec(code_change(OldVsn :: term() | {down, term()}, State :: #state{},
    Extra :: term()) ->
  {ok, NewState :: #state{}} | {error, Reason :: term()}).
code_change(_OldVsn, State, _Extra) ->
  {ok, State}.

%%%===================================================================
%%% Internal functions
%%%===================================================================

format_exception(#'InvalidRequestError'{request = Value, reason = Reason}) ->
  erlang:iolist_to_binary(
    io_lib:format("~nException: InvalidRequestError~nRequest: ~p~nReason: ~s~n", [Value, Reason])
  );

format_exception(#'InvalidAuthError'{auth  = Value, reason = Reason}) ->
  erlang:iolist_to_binary(
    io_lib:format("~nException: InvalidAuthError~nAuth: ~p~nReason: ~s~n", [Value, Reason])
  );

format_exception(#'InvalidUidError'{uid  = Value, reason = Reason}) ->
  erlang:iolist_to_binary(
    io_lib:format("~nException: InvalidUidError~nUID: ~p~nReason: ~s~n", [Value, Reason])
  );

format_exception(Exception) ->
  erlang:iolist_to_binary(
    io_lib:format("~nException: ~p~n", [Exception])
  ).

format_reply(#'BankClient'{data = Data, type = Type, guid = Guid, balance = Balance}) ->
  {AccountType, _} = lists:keyfind(Type, 2, bank_types:enum_info('AccountType')),
  erlang:iolist_to_binary(
    io_lib:format("~nName: ~s~nSurname: ~s~nPesel: ~p~nGUID: ~s~nIncome: ~p~nBalance: ~p~nAccount Type: ~p~n", [
      Data#'UserData'.name,
      Data#'UserData'.surname,
      Data#'UserData'.uid#'UID'.value,
      Guid#'GUID'.value,
      Data#'UserData'.income#'Money'.value,
      Balance#'Money'.value,
      AccountType
    ])
  );

format_reply(#'Money'{value = Balance}) ->
  erlang:iolist_to_binary(
    io_lib:format("~nBalance: ~p~n", [Balance])
  );

format_reply(#'LoanResponse'{accepted = false}) ->
  erlang:iolist_to_binary(
    io_lib:format("~nLoan Request: DENIED~n", [])
  );

format_reply(#'LoanResponse'{baseCost = BaseCost, targetCost = TargetCost}) ->
  erlang:iolist_to_binary(
    io_lib:format("~nLoan Request: ACCEPTED~nBase Cost: ~p~nTargetCost: ~p~n", [
      BaseCost#'Money'.value,
      TargetCost#'Money'.value
    ])
  );

format_reply(Reply) ->
  erlang:iolist_to_binary(
    io_lib:format("~nReply: ~p~n", [Reply])
  ).
