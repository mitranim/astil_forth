-- BOT-ASSISTED

local ffi = require("ffi")
local bit = require("bit")

local CAP = 65536
local RUNS = 2048
local OFFSET = 0xcbf29ce484222325ULL
local PRIME = 0x100000001b3ULL
local WANT = 0xb0a1ea8560222325ULL
local buf = ffi.new("uint8_t[?]", CAP)
local pat = "0123456789abcdef"
local int64 = ffi.typeof("int64_t")

for ind = 0, CAP - 1 do
  buf[ind] = pat:byte((ind % 16) + 1)
end

local function fnv1a64(hash, src, len)
  for ind = 0, len - 1 do
    local low = tonumber(hash % 256ULL)
    hash = (hash + int64(bit.bxor(low, src[ind]) - low)) * PRIME
  end
  return hash
end

local function fmt(hash)
  local high = tonumber(hash / 0x100000000ULL)
  local low = tonumber(hash % 0x100000000ULL)
  return string.format("0x%08x%08x", high, low)
end

local hash = OFFSET
for _ = 1, RUNS do
  hash = fnv1a64(hash, buf, CAP)
end
if hash ~= WANT then
  error("mismatch: expected " .. fmt(WANT) .. "; got " .. fmt(hash))
end
