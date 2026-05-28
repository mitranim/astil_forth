# BOT-TRANSLATED

def fib(depth):
  prev = 0
  next = 1

  while depth > 0:
    prev, next = next, (prev + next)
    depth -= 1

  return next

def run(depth, runs):
  while runs > 0:
    fib(depth)
    runs -= 1
  # print(fib(depth))

run(91, 1 << 16)
