// BOT-GENERATED

package main

import (
	"math/big"
	"sync/atomic"
)

var depthInput int64 = 184
var runsInput int64 = 1 << 21

func fib(depth int) *big.Int {
	prev := new(big.Int)
	next := big.NewInt(1)

	for depth > 0 {
		prev.Add(prev, next)
		prev, next = next, prev
		depth--
	}

	return next
}

func run(depth, runs int) {
	out := new(big.Int)

	for runs > 0 {
		out = fib(depth)
		runs--
	}

	want, ok := new(big.Int).SetString(
		"205697230343233228174223751303346572685",
		10,
	)
	if !ok {
		panic("invalid expected big integer")
	}
	if out.Cmp(want) != 0 {
		panic("big iterative Fibonacci output mismatch")
	}
}

func main() {
	// Atomics are used to make these constants opaque to the compiler.
	run(
		int(atomic.LoadInt64(&depthInput)),
		int(atomic.LoadInt64(&runsInput)),
	)
}
