// BOT-TRANSLATED (with tweaks).

#include <stdint.h>
#include <stdlib.h>

static constexpr uint16_t RUNS = 16384;
static constexpr uint16_t CAP  = 8192;

static int64_t escape(int64_t val) {
  register int64_t x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0));
  return x0;
}

static void escape_ptr(void *val) {
  register void *x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0) : : "memory");
}

static void reset(uint8_t *flags, int32_t cap) {
  for (int32_t ind = 0; ind < cap; ind++) {
    flags[ind] = 1;
  }
}

static int64_t find_prime(uint8_t *flags, int32_t cap) {
  reset(flags, cap);

  int64_t num  = 0;
  int64_t step = 3;

  for (int32_t ind = 0; ind < cap; ind++) {
    if (flags[ind]) {
      for (int64_t ind1 = ind + step; ind1 < cap; ind1 += step) {
        flags[ind1] = 0;
      }
      num++;
    }

    step += 2;
  }

  escape_ptr(flags);
  return num;
}

int main() {
  uint8_t flags[CAP];
  int64_t out = 0;

  for (int32_t ind = 0; ind < RUNS; ind++) {
    out = escape(find_prime(flags, CAP));
  }
  if (out != 1899) abort();
}
