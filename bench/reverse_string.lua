-- BOT-TRANSLATED

local bit = require("bit")

local function string_chars(str)
  local out = {}
  for ind = 1, #str do
    out[ind] = str:sub(ind, ind)
  end
  return out
end

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
  reverse(str)
  if table.concat(str) ~= "fedcba9876543210" then
    error("reverse-string output mismatch")
  end
end

run(string_chars("0123456789abcdef"), bit.lshift(1, 25))
