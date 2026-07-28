% BOT-GENERATED

-module(fnv1a64).
-export([main/0]).

% Kept: bytewise tail recursion; preserves exact 64-bit masking cheaply.
% Rejected: eight-byte source unrolling; it regressed wall time.

-define(OFFSET, 16#cbf29ce484222325).
-define(PRIME, 16#100000001b3).
-define(MASK, 16#ffffffffffffffff).

% Mask every step because Erlang integers do not overflow at 64 bits.
hash(<<>>, Hash) -> Hash;
hash(<<Byte, Rest/binary>>, Hash0) ->
  Hash = (((Hash0 bxor Byte) * ?PRIME) band ?MASK),
  hash(Rest, Hash).

repeat(0, _Input, Hash) -> Hash;
repeat(Runs, Input, Hash) ->
  repeat(Runs - 1, Input, hash(Input, Hash)).

main() ->
  Input = binary:copy(<<"0123456789abcdef">>, 4096),
  16#b0a1ea8560222325 = repeat(2048, Input, ?OFFSET),
  ok.
