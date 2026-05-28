// BOT-TRANSLATED (with tweaks).

#include <stdint.h>

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

int main(void) {
  const uint64_t count = 1 << 16;
  for (uint64_t ind = 0; ind < escape(count); ind++) {
    escape(fib(escape(91)));
  }
  // printf("%llu\n", fib(91));
}
