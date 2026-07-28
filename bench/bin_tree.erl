% BOT-GENERATED

-module(bin_tree).
-export([main/0]).

% Kept: recursive tuples; fastest measured Erlang tree representation.
% Alternatives: packed binary and atomics remain distinct storage workloads.

make(0) -> {nil, nil};
make(Depth) ->
  Next = Depth - 1,
  {make(Next), make(Next)}.

count(nil) -> 0;
count({Left, Right}) -> 1 + count(Left) + count(Right).

trees(0, _Depth, Count) -> Count;
trees(Runs, Depth, Count) ->
  trees(Runs - 1, Depth, Count + count(make(Depth))).

depth_lines(Depth, MaxDepth, _MinDepth) when Depth > MaxDepth -> [];
depth_lines(Depth, MaxDepth, MinDepth) ->
  Iters = 1 bsl (MaxDepth - Depth + MinDepth),
  Count = trees(Iters, Depth, 0),
  [
    io_lib:format(
      "~B trees of depth ~B; count: ~B~n",
      [Iters, Depth, Count]
    )
    | depth_lines(Depth + 2, MaxDepth, MinDepth)
  ].

expected() ->
  <<
    "stretch tree of depth 19; count: 1048575\n",
    "262144 trees of depth 4; count: 8126464\n",
    "65536 trees of depth 6; count: 8323072\n",
    "16384 trees of depth 8; count: 8372224\n",
    "4096 trees of depth 10; count: 8384512\n",
    "1024 trees of depth 12; count: 8387584\n",
    "256 trees of depth 14; count: 8388352\n",
    "64 trees of depth 16; count: 8388544\n",
    "16 trees of depth 18; count: 8388592\n",
    "long lived tree of depth 18; count: 524287\n"
  >>.

main() ->
  MinDepth = 4,
  MaxDepth = 18,
  StretchDepth = MaxDepth + 1,
  LongLived = make(MaxDepth),
  Out = iolist_to_binary([
    io_lib:format(
      "stretch tree of depth ~B; count: ~B~n",
      [StretchDepth, count(make(StretchDepth))]
    ),
    depth_lines(MinDepth, MaxDepth, MinDepth),
    io_lib:format(
      "long lived tree of depth ~B; count: ~B~n",
      [MaxDepth, count(LongLived)]
    )
  ]),
  Out = expected(),
  ok.
