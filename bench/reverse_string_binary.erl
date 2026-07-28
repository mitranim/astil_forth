% BOT-GENERATED

-module(reverse_string_binary).
-export([main/0]).

% Kept: binary/list BIF pipeline; fastest measured binary-preserving path.
% Rejected: bytewise binary prepending; repeated construction doubled time.

reverse(Value) ->
  list_to_binary(lists:reverse(binary_to_list(Value))).

repeat(0, Value) -> Value;
repeat(Runs, Value) -> repeat(Runs - 1, reverse(Value)).

main() ->
  Reversed = reverse(repeat(1 bsl 25, <<"0123456789abcdef">>)),
  <<"fedcba9876543210">> = Reversed,
  ok.
