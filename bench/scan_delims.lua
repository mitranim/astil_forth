-- BOT-ASSISTED

local ffi = require("ffi")
local CAP = 2 ^ 16
local RUNS = (2 ^ 27) / (CAP / 16)
local WANT = (2 ^ 27) * 9
local pat = "{a,b:c[d]e} \n\tfg"
local buf = ffi.new("uint8_t[?]", CAP)
local dels = ffi.new("uint8_t[256]")
local LBRACE = string.byte("{")
local RBRACE = string.byte("}")
local LBRACK = string.byte("[")
local RBRACK = string.byte("]")
local COLON = string.byte(":")
local COMMA = string.byte(",")
local SPACE = string.byte(" ")
local LF = string.byte("\n")
local TAB = string.byte("\t")

dels[LBRACE] = 1
dels[RBRACE] = 1
dels[LBRACK] = 1
dels[RBRACK] = 1
dels[COLON] = 1
dels[COMMA] = 1
dels[SPACE] = 1
dels[LF] = 1
dels[TAB] = 1

for ind = 0, CAP - 1 do buf[ind] = pat:byte((ind % #pat) + 1) end

local function scan(buf, cap, dels)
  local out = 0
  for ind = 0, cap - 1 do
    out = out + dels[buf[ind]]
  end
  return out
end

local out = 0
for _ = 1, RUNS do out = out + scan(buf, CAP, dels) end
if out ~= WANT then error("mismatch: expected " .. WANT .. "; got " .. out) end
