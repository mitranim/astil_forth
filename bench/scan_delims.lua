-- BOT-GENERATED

local ffi = require("ffi")
local CAP = 2 ^ 16
local REPS = (2 ^ 24) / (CAP / 16)
local WANT = (2 ^ 24) * 9
local pat = ffi.new("uint8_t[16]", {123, 97, 44, 98, 58, 99, 91, 100,
                                    93, 101, 125, 32, 10, 9, 102, 103})
local buf = ffi.new("uint8_t[?]", CAP)
for i = 0, CAP - 1 do buf[i] = pat[bit.band(i, 15)] end

local function is_delim(c)
  return c == 123 or c == 125 or c == 91 or c == 93 or c == 58 or
         c == 44 or c == 32 or c == 10 or c == 9
end

local out = 0
for _ = 1, REPS do
  for j = 0, CAP - 1 do
    if is_delim(buf[j]) then out = out + 1 end
  end
end
if os.getenv("SCAN_DELIMS_PRINT") then print(out) end
if out ~= WANT then os.exit(1) end
