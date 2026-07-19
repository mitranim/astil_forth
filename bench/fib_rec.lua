local function fib(ind)
  if ind <= 1 then return 1 end
  return fib(ind - 1) + fib(ind - 2)
end

if fib(39) ~= 102334155 then
  error("recursive Fibonacci output mismatch")
end
