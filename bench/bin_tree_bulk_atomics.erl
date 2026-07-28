% BOT-GENERATED

-module(bin_tree_bulk_atomics).
-export([main/0]).

% Kept: atomics; mutable flat storage with lower RSS than packed binary.
% Rejected: counters; both option sets increased wall time.

tree_length(Depth) -> (1 bsl (Depth + 1)) - 1.

left_field(Node) -> Node * 2 - 1.
right_field(Node) -> Node * 2.

init(_Ref, Node, InternalCount) when Node > InternalCount -> ok;
init(Ref, Node, InternalCount) ->
  ok = atomics:put(Ref, left_field(Node), Node * 2),
  ok = atomics:put(Ref, right_field(Node), Node * 2 + 1),
  init(Ref, Node + 1, InternalCount).

make(Depth) ->
  Length = tree_length(Depth),
  Ref = atomics:new(Length * 2, []),
  ok = init(Ref, 1, Length div 2),
  Ref.

count(_Ref, 0) -> 0;
count(Ref, Node) ->
  Left = atomics:get(Ref, left_field(Node)),
  Right = atomics:get(Ref, right_field(Node)),
  1 + count(Ref, Left) + count(Ref, Right).

trees(0, _Depth, Count) -> Count;
trees(Runs, Depth, Count) ->
  Tree = make(Depth),
  trees(Runs - 1, Depth, Count + count(Tree, 1)).

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
  Stretch = make(StretchDepth),
  Out = iolist_to_binary([
    io_lib:format(
      "stretch tree of depth ~B; count: ~B~n",
      [StretchDepth, count(Stretch, 1)]
    ),
    depth_lines(MinDepth, MaxDepth, MinDepth),
    io_lib:format(
      "long lived tree of depth ~B; count: ~B~n",
      [MaxDepth, count(LongLived, 1)]
    )
  ]),
  Out = expected(),
  ok.
