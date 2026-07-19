function fib(depth) {
  let prev = 0n
  let next = 1n

  while (depth-- > 0) {
    const tmp = prev + next
    prev = next
    next = tmp
  }

  return next
}

function run(depth, runs) {
  let out = 0n
  while (runs-- > 0) out = fib(depth)
  if (out !== 205697230343233228174223751303346572685n) {
    throw Error("big iterative Fibonacci output mismatch")
  }
}

run(184, 1 << 21)
