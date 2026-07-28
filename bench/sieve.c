// BOT-TRANSLATED (with tweaks).

#include "./util.c"
#include <stdint.h>
#include <stdlib.h>

static constexpr uint16_t RUNS = 16384;
static constexpr uint16_t CAP  = 8192;

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
  uint8_t       flags[CAP];
  const int32_t cap  = (int32_t)escape_u64(CAP);
  const int32_t runs = (int32_t)escape_u64(RUNS);
  int64_t       out  = 0;

  for (int32_t ind = 0; ind < runs; ind++) {
    out = (int64_t)escape_u64((uint64_t)find_prime(flags, cap));
  }
  if (out != 1899) abort();
}
