#include "./util.c"
#include <stdint.h>
#include <stdlib.h>

static int64_t fib(int64_t src) {
  if (src <= 1) return 1;
  return fib(src - 1) + fib(src - 2);
}

// #include <stdio.h>

int main() {
  const int64_t count = (int64_t)escape_u64(39);
  if ((int64_t)escape_u64((uint64_t)fib(count)) != 102334155) abort();
}
