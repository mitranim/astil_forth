% BOT-GENERATED

-module(fib_rec).
-export([main/0]).

% Kept: direct branching recursion; measured call count matches the workload.
% Rejected: serial runtime flags; no broad or dramatic gain.

fib(N) when N =< 1 -> 1;
fib(N) -> fib(N - 1) + fib(N - 2).

main() ->
  102334155 = fib(39),
  ok.
