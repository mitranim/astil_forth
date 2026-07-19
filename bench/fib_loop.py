# BOT-TRANSLATED

def fib(depth):
  prev = 0
  next = 1

  while depth > 0:
    prev, next = next, (prev + next)
    depth -= 1

  return next

def run(depth, runs):
  out = 0
  while runs > 0:
    out = fib(depth)
    runs -= 1
  if out != 7540113804746346429:
    raise RuntimeError("iterative Fibonacci output mismatch")

run(91, 1 << 22)
