#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int64_t fib(int64_t src) {
  if (src <= 1) return 1;
  return fib(src - 1) + fib(src - 2);
}

static int64_t escape(int64_t val) {
  register int64_t x0 __asm__("x0") = val;
  __asm__("" : "+r"(x0));
  return x0;
}

int main(void) {
  const int64_t count = 36;
  escape(fib(escape(count)));
  // printf("fib(%lld): %lld\n", count, fib(count));
}
