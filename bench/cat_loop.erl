% BOT-GENERATED

-module(cat_loop).
-export([main/0]).

% Kept: 1 MiB raw reads and direct fd-port writes; exact and fastest measured.
% Rejected: group-leader output, file:copy, io:put_chars, and smaller buffers.

copy(Input, Output) ->
  case file:read(Input, 1024 * 1024) of
    {ok, Data} ->
      true = port_command(Output, Data),
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
  try
    Stdout = open_port({fd, 0, 1}, [out, binary]),
    try
      Args = init:get_plain_arguments(),
      Names = case Args of [] -> ["-"]; _ -> Args end,
      ok = lists:foreach(
        fun(Name) -> ok = copy_name(Name, Stdin, Stdout) end,
        Names
      )
    after
      port_close(Stdout)
    end
  after
    file:close(Stdin)
  end.
