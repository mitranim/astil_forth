-- BOT-TRANSLATED

local bit = require("bit")

local function reverse(str)
  local low = 1
  local high = #str
  while low < high do
    str[low], str[high] = str[high], str[low]
    low = low + 1
    high = high - 1
  end
end

local function run(str, runs)
  for _ = 1, runs do
    reverse(str)
  end
  -- print(table.concat(str))
end

run({"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f"}, bit.lshift(1, 22) + 1)
