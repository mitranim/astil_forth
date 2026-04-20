#pragma once

#define file_write_val(tar, ...)                   \
  ({                                               \
    const auto tmp_val = __VA_ARGS__;              \
    file_write(tar, &tmp_val, sizeof(tmp_val), 1); \
  })
