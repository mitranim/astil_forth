#pragma once
#include "./num.h"

typedef Uint Hash;

#define hash_val(val)                          \
  ({                                           \
    const auto tmp = (val);                    \
    hash_bytes((const U8 *)&tmp, sizeof(tmp)); \
  })
