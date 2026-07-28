% BOT-GENERATED

-module(bubble_atomics).
-export([main/0]).

% Kept: carry-based atomics pass; one read per comparison.
% Rejected: rereading both neighbors; redundant synchronization cost.

% atomics supplies the mutable 64-bit storage used by this variant.
pseudo_random(Seed) -> (Seed * 1309 + 13849) band 65535.

init(_Ref, Index, Length, _Seed) when Index > Length -> ok;
init(Ref, Index, Length, Seed0) ->
  Seed = pseudo_random(Seed0),
  ok = atomics:put(Ref, Index, Seed),
  init(Ref, Index + 1, Length, Seed).

pass(Ref, Ceiling) ->
  pass(Ref, 2, Ceiling, atomics:get(Ref, 1)).

pass(Ref, Index, Ceiling, Carry) when Index > Ceiling ->
  ok = atomics:put(Ref, Ceiling, Carry);
pass(Ref, Index, Ceiling, Carry) ->
  Right = atomics:get(Ref, Index),
  case Carry > Right of
    true ->
      ok = atomics:put(Ref, Index - 1, Right),
      pass(Ref, Index + 1, Ceiling, Carry);
    false ->
      ok = atomics:put(Ref, Index - 1, Carry),
      pass(Ref, Index + 1, Ceiling, Right)
  end.

sort(_Ref, Ceiling) when Ceiling =< 1 -> ok;
sort(Ref, Ceiling) ->
  ok = pass(Ref, Ceiling),
  sort(Ref, Ceiling - 1).

verify(_Ref, Index, Length) when Index >= Length -> ok;
verify(Ref, Index, Length) ->
  true = atomics:get(Ref, Index) =< atomics:get(Ref, Index + 1),
  verify(Ref, Index + 1, Length).

main() ->
  Length = 32768,
  Ref = atomics:new(Length, []),
  ok = init(Ref, 1, Length, 74755),
  ok = sort(Ref, Length),
  ok = verify(Ref, 1, Length).
