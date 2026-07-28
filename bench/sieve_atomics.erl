% BOT-GENERATED

-module(sieve_atomics).
-export([main/0]).

% Kept: epoch-marked atomics; avoids resetting every flag on every run.
% Rejected: persistent array path-copy and explicit per-run atomic resets.

% atomics supplies reusable mutable flags without foreign code.
% Epoch advances independently; zero-reset only on uint64 wrap.
-define(CAPACITY, 8192).
-define(MAX_EPOCH, 16#ffffffffffffffff).

mark(_Ref, Index0, Capacity, _Step, _Epoch) when Index0 >= Capacity -> ok;
mark(Ref, Index0, Capacity, Step, Epoch) ->
  ok = atomics:put(Ref, Index0 + 1, Epoch),
  mark(Ref, Index0 + Step, Capacity, Step, Epoch).

scan(_Ref, Index0, Capacity, _Step, Count, _Epoch) when Index0 >= Capacity ->
  Count;
scan(Ref, Index0, Capacity, Step, Count0, Epoch) ->
  case atomics:get(Ref, Index0 + 1) of
    Epoch ->
      scan(Ref, Index0 + 1, Capacity, Step + 2, Count0, Epoch);
    _ ->
      ok = mark(Ref, Index0 + Step, Capacity, Step, Epoch),
      scan(Ref, Index0 + 1, Capacity, Step + 2, Count0 + 1, Epoch)
  end.

reset(_Ref, Index) when Index > ?CAPACITY -> ok;
reset(Ref, Index) ->
  ok = atomics:put(Ref, Index, 0),
  reset(Ref, Index + 1).

next_epoch(Ref, ?MAX_EPOCH) ->
  ok = reset(Ref, 1),
  1;
next_epoch(_Ref, Epoch) -> Epoch + 1.

run(0, _Ref, _Epoch, Out) -> Out;
run(Runs, Ref, Epoch, _Out) ->
  Out = scan(Ref, 0, ?CAPACITY, 3, 0, Epoch),
  run(Runs - 1, Ref, next_epoch(Ref, Epoch), Out).

main() ->
  Ref = atomics:new(?CAPACITY, [{signed, false}]),
  1899 = run(16384, Ref, 1, 0),
  ok.
