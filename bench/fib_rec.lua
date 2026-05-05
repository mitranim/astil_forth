function fib(ind)
  if (ind <= 1) then return 1 end
  return fib(ind - 1) + fib(ind - 2)
end

fib(36)
-- print(fib(36))
