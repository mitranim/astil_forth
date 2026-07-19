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
  let out = 0
  while (runs-- > 0) out = fib(depth)
  if (out !== 7540113804746346429) {
    throw Error("iterative Fibonacci output mismatch")
  }
}

// Note: the result is slightly incorrect due to limited integer precision.
run(91, 1 << 22)
