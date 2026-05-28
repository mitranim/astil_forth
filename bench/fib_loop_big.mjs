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
  while (runs-- > 0) fib(depth)
  // console.log(fib(depth))
}

run(234, (1 << 16))
