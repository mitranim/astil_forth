function fib(depth) {
  let prev = 0
  let next = 1

  while (depth-- > 0) {
    const tmp = prev + next
    prev = next
    next = tmp
  }

  return next
}

function run(depth, runs) {
  while (runs-- > 0) fib(depth)
  // console.log(fib(depth))
}

// Note: the result is slightly incorrect due to limited integer precision.
run(91, (1 << 16))
