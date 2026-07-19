// BOT-TRANSLATED (with tweaks).

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

static uint64_t escape(uint64_t val) {
  register uint64_t x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0));
  return x0;
}

// #include <stdio.h>

int main() {
  const uint64_t count = 1 << 22;
  uint64_t out = 0;
  for (uint64_t ind = 0; ind < escape(count); ind++) {
    out = escape(fib(escape(91)));
  }
  if (out != UINT64_C(7540113804746346429)) abort();
}
