% BOT-GENERATED

-module(fib_loop_big).
-export([main/0]).

% Kept: native bignums with accumulator recursion; idiomatic and direct.
% Rejected: serial runtime flags; no broad or dramatic gain.

% Erlang integers grow into native bignums; no manual two-word carry is needed.
fib(0, _Prev, Next) -> Next;
fib(Depth, Prev, Next) -> fib(Depth - 1, Next, Prev + Next).

repeat(0, Out) -> Out;
repeat(Runs, _Out) -> repeat(Runs - 1, fib(184, 0, 1)).

main() ->
  205697230343233228174223751303346572685 =
    repeat(1 bsl 21, 0),
  ok.
