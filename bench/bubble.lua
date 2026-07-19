-- BOT-TRANSLATED (with tweaks).

local bit = require("bit")

local function pseudo_random(seed)
  local val = bit.band(seed * 1309 + 13849, 65535)
  return val, val
end

local function list_init(list, len)
  local seed = 74755
  for ind = 1, len do
    seed, list[ind] = pseudo_random(seed)
  end
end

local function list_verify(list)
  for ind = 1, #list - 1 do
    if list[ind] > list[ind + 1] then
      error("[bubble] not sorted")
    end
  end
end

local function bubble(list)
  for ceil = #list - 1, 1, -1 do
    for ind = 1, ceil do
      if list[ind] > list[ind + 1] then
        list[ind], list[ind + 1] = list[ind + 1], list[ind]
      end
    end
  end
end

local function bubble_sort(len)
  local list = {}
  list_init(list, len)
  bubble(list)
  list_verify(list)
end

bubble_sort(32768)
