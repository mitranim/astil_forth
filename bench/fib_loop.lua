-- BOT-TRANSLATED (with tweaks).

local bit = require("bit")

local function fib(depth)
  local prev = 0
  local next = 1

  for _ = 1, depth do
    prev, next = next, prev + next
  end

  return next
end

local function run(depth, runs)
  for _ = 1, runs do
    fib(depth)
  end
  -- print(string.format("%f", fib(depth)))
end

run(91, bit.lshift(1, 20))
