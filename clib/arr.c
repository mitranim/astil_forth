#pragma once
#include "./err.c"
#include "./num.h"
#include <string.h>

static void arr_set_impl(void *arr, Ind cap, Ind ind, const void *src, Ind size) {
  aver(((U8 *)arr + ind + size) < ((U8 *)arr + cap));
  memcpy((U8 *)arr + ind, src, size);
}
