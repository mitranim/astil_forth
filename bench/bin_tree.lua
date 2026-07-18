-- BOT-TRANSLATED from reg-CC file.

local function node_tree_make(depth)
  if depth == 0 then return {} end

  depth = depth - 1
  return {node_tree_make(depth), node_tree_make(depth)}
end

local function node_tree_count(node)
  local left = node[1]
  if left == nil then return 1 end
  return 1 + node_tree_count(left) + node_tree_count(node[2])
end

local min_depth = 4
local max_depth = 18
local stretch_depth = max_depth + 1

local out = {}

local long_lived_tree = node_tree_make(max_depth)

do
  local tree = node_tree_make(stretch_depth)
  out[#out + 1] = string.format(
    "stretch tree of depth %d; count: %d\n",
    stretch_depth,
    node_tree_count(tree)
  )
end

for depth = min_depth, max_depth, 2 do
  local count = 0
  local iters = 2 ^ (max_depth - depth + min_depth)

  for _ = 1, iters do
    count = count + node_tree_count(node_tree_make(depth))
  end

  out[#out + 1] = string.format(
    "%d trees of depth %d; count: %d\n",
    iters,
    depth,
    count
  )
end

out[#out + 1] = string.format(
  "long lived tree of depth %d; count: %d\n",
  max_depth,
  node_tree_count(long_lived_tree)
)

local expected = [[stretch tree of depth 19; count: 1048575
262144 trees of depth 4; count: 8126464
65536 trees of depth 6; count: 8323072
16384 trees of depth 8; count: 8372224
4096 trees of depth 10; count: 8384512
1024 trees of depth 12; count: 8387584
256 trees of depth 14; count: 8388352
64 trees of depth 16; count: 8388544
16 trees of depth 18; count: 8388592
long lived tree of depth 18; count: 524287
]]

local actual = table.concat(out)
if actual ~= expected then error(actual) end
