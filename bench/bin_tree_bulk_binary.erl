% BOT-GENERATED

-module(bin_tree_bulk_binary).
-export([main/0]).

% Kept: append-built packed binary with direct recursive traversal.
% Rejected: iolist construction and an explicit list stack; both were slower.

tree_length(Depth) -> (1 bsl (Depth + 1)) - 1.

build(Node, Length, _InternalCount, Out) when Node > Length -> Out;
build(Node, Length, InternalCount, Out0) ->
  {Left, Right} =
    case Node =< InternalCount of
      true -> {Node * 2, Node * 2 + 1};
      false -> {0, 0}
    end,
  Out = <<
    Out0/binary,
    Left:64/native-unsigned-integer,
    Right:64/native-unsigned-integer
  >>,
  build(Node + 1, Length, InternalCount, Out).

make(Depth) ->
  Length = tree_length(Depth),
  build(1, Length, Length div 2, <<>>).

count(_Tree, 0) -> 0;
count(Tree, Node) ->
  Offset = (Node - 1) * 16,
  <<
    _:Offset/binary,
    Left:64/native-unsigned-integer,
    Right:64/native-unsigned-integer,
    _/binary
  >> = Tree,
  1 + count(Tree, Left) + count(Tree, Right).

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
