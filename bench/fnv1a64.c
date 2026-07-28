// BOT-ASSISTED

#include "./util.c"
#include <stddef.h>
#include <stdint.h>

static constexpr uint32_t CAP  = 65536;
static constexpr uint16_t RUNS = 2048;

static uint8_t buf[CAP];

static void init(size_t cap) {
  static const uint8_t pat[] = "0123456789abcdef";
  for (size_t ind = 0; ind < cap; ind++) {
    buf[ind] = pat[ind & 15];
  }
}

static uint64_t fnv1a64(uint64_t hash, const uint8_t *src, size_t len) {
  for (size_t ind = 0; ind < len; ind++) {
    hash ^= src[ind];
    hash *= UINT64_C(0x100000001B3);
  }
  return hash;
}

int main(void) {
  const size_t cap  = (size_t)escape_u64(CAP);
  const int    runs = (int)escape_u64(RUNS);

  init(cap);

  const uint8_t *const input = escape_ptr(buf);
  uint64_t             hash  = UINT64_C(0xCBF29CE484222325);

  for (int rep = 0; rep < runs; rep++) {
    hash = fnv1a64(hash, input, cap);
  }
  return hash == UINT64_C(0xB0A1EA8560222325) ? 0 : 1;
}
