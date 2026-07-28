// BOT-TRANSLATED

#include "./util.c"
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

int main() {
  const uint64_t count = escape_u64(1 << 21);
  const uint64_t depth = escape_u64(184);
  U128           out   = 0;

  for (uint64_t ind = 0; ind < count; ind++) {
    out = escape_u128(fib(depth));
  }

  const U128 want = ((U128)UINT64_C(0x9ABFD87547C0E48C) << 64) |
    UINT64_C(0x30173357E778CD8D);

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
