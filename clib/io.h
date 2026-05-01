#pragma once
#include "./misc.h"

#define file_write_val_inner(tmp, tar, ...) \
  ({                                        \
    const auto tmp = __VA_ARGS__;           \
    file_write(tar, &tmp, sizeof(tmp), 1);  \
  })

#define file_write_val(...) file_write_val_inner(UNIQ_IDENT, __VA_ARGS__)
