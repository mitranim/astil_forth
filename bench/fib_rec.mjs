// BOT-TRANSLATED

function fib(ind) {
  if (ind <= 1) return 1
  return fib(ind - 1) + fib(ind - 2)
}

if (fib(39) !== 102334155) {
  throw Error("recursive Fibonacci output mismatch")
}
