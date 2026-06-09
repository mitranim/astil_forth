// BOT-TRANSLATED

#include <stdint.h>

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
  __asm__ volatile("" : "+r"(val));
  return val;
}

// #include <stdio.h>
//
// static void print_u128(U128 val) {
//   char buf[40];
//   char *ptr = buf + sizeof(buf);
//
//   if (!val) {
//     putchar('0');
//     return;
//   }
//
//   do {
//     *--ptr = '0' + (val % 10);
//     val /= 10;
//   } while (val);
//
//   while (ptr < buf + sizeof(buf)) putchar(*ptr++);
// }

int main() {
  const uint64_t count = escape(1 << 18);
  for (uint64_t ind = 0; ind < count; ind++) {
    escape128(fib(escape(184)));
  }
  // print_u128(fib(184));
  // putchar('\n');
}
