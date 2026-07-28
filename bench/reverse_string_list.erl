% BOT-GENERATED

-module(reverse_string_list).
-export([main/0]).

% Kept: lists:reverse/1; fastest idiomatic Erlang string-list path.
% Alternative: the slower binary row remains for binary-string semantics.

repeat(0, Value) -> Value;
repeat(Runs, Value) -> repeat(Runs - 1, lists:reverse(Value)).

main() ->
  Reversed = lists:reverse(repeat(1 bsl 25, "0123456789abcdef")),
  "fedcba9876543210" = Reversed,
  ok.
