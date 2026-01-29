function fib(ind) {
  if (ind <= 1) return 1
  return fib(ind - 1) + fib(ind - 2)
}

fib(36)

// console.log(fib(36))
