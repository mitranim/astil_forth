// BOT-TRANSLATED

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned __int128 U128;

static U128 fib(uint64_t depth) {
  U128 prev = 0;
  U128 next = 1;

  for (uint32_t ind = 0; ind < depth; ind++) {
    const U128 tmp = prev + next;
    prev           = next;
    next           = tmp;
  }

  return next;
}

static uint64_t escape(uint64_t val) {
  register uint64_t x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0));
  return x0;
}

static U128 escape128(U128 val) {
  uint64_t low = (uint64_t)val;
  uint64_t high = (uint64_t)(val >> 64);
  __asm__ volatile("" : "+r"(low), "+r"(high));
  return ((U128)high << 64) | low;
}

int main() {
  const uint64_t count = escape(1 << 21);
  U128 out = 0;
  for (uint64_t ind = 0; ind < count; ind++) {
    out = escape128(fib(escape(184)));
  }
  const U128 want =
    ((U128)UINT64_C(0x9abfd87547c0e48c) << 64) |
    UINT64_C(0x30173357e778cd8d);
  if (out != want) {
    fprintf(
      stderr,
      "big iterative Fibonacci mismatch: %016llx%016llx\n",
      (unsigned long long)(out >> 64),
      (unsigned long long)out
    );
    abort();
  }
}
