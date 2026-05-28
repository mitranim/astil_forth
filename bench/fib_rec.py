def fib(ind):
  if ind <= 1:
    return 1
  return fib(ind - 1) + fib(ind - 2)

fib(36)

# print(fib(36))
