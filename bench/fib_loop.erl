% BOT-GENERATED

-module(fib_loop).
-export([main/0]).

% Kept: accumulator recursion; BEAM emits a tail call.
% Rejected: serial runtime flags; no broad or dramatic gain.

% Erlang's idiomatic loop is tail recursion with accumulator arguments.
fib(0, _Prev, Next) -> Next;
fib(Depth, Prev, Next) -> fib(Depth - 1, Next, Prev + Next).

repeat(0, Out) -> Out;
repeat(Runs, _Out) -> repeat(Runs - 1, fib(91, 0, 1)).

main() ->
  7540113804746346429 = repeat(1 bsl 22, 0),
  ok.
