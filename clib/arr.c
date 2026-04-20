#pragma once
#include "./err.c"
#include "./num.h"
#include <string.h>

static void arr_set_impl(
  void *arr, Uint cap, Ind ind, const void *src, Uint size
) {
  aver(((U8 *)arr + ind + size) < ((U8 *)arr + cap));
  memcpy((U8 *)arr + ind, src, size);
}
