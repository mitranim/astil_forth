// BOT-TRANSLATED (with tweaks).

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr int32_t LEN = 32768;

static void escape_ptr(void *val) {
  register void *x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0) : : "memory");
}

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
  int64_t list[LEN];

  list_init(list, LEN);
  bubble(list, LEN);
  escape_ptr(list);
  list_verify(list, LEN);
}
