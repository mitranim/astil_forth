% BOT-GENERATED

-module(tcp_server_passive).
-export([main/0]).

% Kept: passive process-per-connection I/O; lowest measured Erlang TCP RSS.
% Rejected: active-once; dominated. Socket backend traded CPU for RSS and wall.

handle(Socket) ->
  ok = gen_tcp:send(Socket, <<"R">>),
  loop(Socket).

loop(Socket) ->
  case gen_tcp:recv(Socket, 1) of
    {ok, <<"D">>} ->
      ok = gen_tcp:send(Socket, <<"D">>),
      loop(Socket);
    {ok, <<"Q">>} ->
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
