// BOT-TRANSLATED (with tweaks).

#include "./util.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr int32_t LEN = 32768;

static int64_t pseudo_random(int64_t seed) {
  return (seed * 1309 + 13849) & 65535;
}

static void list_init(int64_t *list, int32_t len) {
  int64_t seed = 74755;
  for (int32_t ind = 0; ind < len; ind++) {
    list[ind] = seed = pseudo_random(seed);
  }
}

static void list_verify(const int64_t *list, int32_t len) {
  for (int32_t ind = 0; ind < len - 1; ind++) {
    if (list[ind] > list[ind + 1]) {
      fprintf(stderr, "[bubble] not sorted\n");
      abort();
    }
  }
}

static void bubble(int64_t *list, int32_t len) {
  for (int32_t ceil = len - 1; ceil > 0; ceil--) {
    for (int32_t ind = 0; ind < ceil; ind++) {
      if (list[ind] > list[ind + 1]) {
        const int64_t tmp = list[ind];
        list[ind]         = list[ind + 1];
        list[ind + 1]     = tmp;
      }
    }
  }
}

int main() {
  int64_t       list[LEN];
  const int32_t len = (int32_t)escape_u64(LEN);

  list_init(list, len);
  bubble(list, len);
  escape_ptr(list);
  list_verify(list, len);
}
