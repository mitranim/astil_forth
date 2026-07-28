% BOT-GENERATED

-module(cat_loop).
-export([main/0]).

% Kept: 1 MiB raw reads and file:write; fastest exact byte stream.
% Rejected: file:copy, io:put_chars, smaller buffers, and runtime flags.

copy(Input, Output) ->
  case file:read(Input, 1024 * 1024) of
    {ok, Data} ->
      ok = file:write(Output, Data),
      copy(Input, Output);
    eof -> ok
  end.

copy_name("-", Stdin, Stdout) -> copy(Stdin, Stdout);
copy_name(Name, _Stdin, Stdout) ->
  {ok, Input} = file:open(Name, [read, raw, binary]),
  try copy(Input, Stdout)
  after ok = file:close(Input)
  end.

main() ->
  {ok, Stdin} = file:open("/dev/stdin", [read, raw, binary]),
  Stdout = group_leader(),
  try
    Args = init:get_plain_arguments(),
    Names = case Args of [] -> ["-"]; _ -> Args end,
    ok = lists:foreach(
      fun(Name) -> ok = copy_name(Name, Stdin, Stdout) end,
      Names
    )
  after
    ok = file:close(Stdin)
  end.
