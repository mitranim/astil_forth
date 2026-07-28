% BOT-GENERATED

-module(tcp_server_active_n).
-export([main/0]).

% Kept: process per connection with bounded active-N receive batches.
% Rejected: active-once and socket backend; CPU or RSS regressed.

-define(ACTIVE_BATCH, 64).

handle(Socket) ->
  ok = inet:setopts(Socket, [{active, ?ACTIVE_BATCH}]),
  ok = gen_tcp:send(Socket, <<"R">>),
  loop(Socket).

loop(Socket) ->
  receive
    {tcp, Socket, <<"D">>} ->
      ok = gen_tcp:send(Socket, <<"D">>),
      loop(Socket);
    {tcp_passive, Socket} ->
      ok = inet:setopts(Socket, [{active, ?ACTIVE_BATCH}]),
      loop(Socket);
    {tcp, Socket, <<"Q">>} ->
      ok = gen_tcp:send(Socket, <<"Q">>),
      ok = gen_tcp:close(Socket)
  end.

start_handler(Socket) ->
  Pid = spawn(fun() ->
    receive {socket, Owned} -> handle(Owned) end
  end),
  ok = gen_tcp:controlling_process(Socket, Pid),
  Pid ! {socket, Socket},
  ok.

accept(Listen) ->
  {ok, Socket} = gen_tcp:accept(Listen),
  ok = start_handler(Socket),
  accept(Listen).

main() ->
  {ok, Listen} = gen_tcp:listen(
    19777,
    [binary, {packet, raw}, {active, false}, {reuseaddr, true},
     {ip, {127, 0, 0, 1}}, {backlog, 4096}]
  ),
  accept(Listen).
