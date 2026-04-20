#pragma once
#include "./num.h"

typedef Uint Fnv_hash;

#define hash_val(val)                              \
  ({                                               \
    const auto tmp = (val);                        \
    fnv_hash_bytes((const U8 *)&tmp, sizeof(tmp)); \
  })
