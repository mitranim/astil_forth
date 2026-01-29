#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int64_t fib(int64_t src) {
  if (src <= 1) return 1;
  return fib(src - 1) + fib(src - 2);
}

int main(int argc, const char **argv) {
  const auto num = strtol(argv[1], nullptr, 10);
  printf("fib(%ld): %lld\n", num, fib(num));
}
