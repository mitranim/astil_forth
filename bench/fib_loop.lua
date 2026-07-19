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
  local out = 0
  for _ = 1, runs do
    out = fib(depth)
  end
  if out ~= 7540113804746346429 then
    error("iterative Fibonacci output mismatch")
  end
end

run(91, bit.lshift(1, 22))
