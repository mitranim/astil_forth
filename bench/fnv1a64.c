// BOT-ASSISTED

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr uint32_t CAP  = 65536;
static constexpr uint16_t RUNS = 512;

static uint8_t buf[CAP];

static void init(void) {
  static const uint8_t pat[] = "0123456789abcdef";
  for (size_t ind = 0; ind < CAP; ind++) {
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
  init();
  uint64_t hash = UINT64_C(0xCBF29CE484222325);
  for (int rep = 0; rep < RUNS; rep++) {
    hash = fnv1a64(hash, buf, CAP);
  }
  if (getenv("FNV1A64_PRINT")) {
    printf("0x%" PRIx64 "\n", hash);
  }
  return hash == UINT64_C(0x8D8A704CBB222325) ? 0 : 1;
}
