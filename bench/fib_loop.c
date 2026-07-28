// BOT-TRANSLATED (with tweaks).

#include "./util.c"
#include <stdint.h>
#include <stdlib.h>

static uint64_t fib(uint64_t cap) {
  uint64_t prev = 0;
  uint64_t next = 1;

  for (uint32_t ind = 0; ind < cap; ind++) {
    const uint64_t tmp = prev + next;
    prev               = next;
    next               = tmp;
  }

  return next;
}

// #include <stdio.h>

int main() {
  const uint64_t count = escape_u64(1 << 22);
  const uint64_t depth = escape_u64(91);
  uint64_t       out   = 0;
  for (uint64_t ind = 0; ind < count; ind++) {
    out = escape_u64(fib(depth));
  }
  if (out != UINT64_C(7540113804746346429)) abort();
}
