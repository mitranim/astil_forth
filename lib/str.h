#pragma once
#include "./err.h"
#include "./num.h"

#define str_buf(buf_len) \
  struct {               \
    Ind  len;            \
    char buf[buf_len];   \
  }

#define str_cap(str) ((Ind)(sizeof((str)->buf) / sizeof((str)->buf[0])))

#define str_trunc(str)              \
  ({                                \
    const auto str_trunc_tmp = str; \
    str_trunc_tmp->len       = 0;   \
    str_terminate(str_trunc_tmp);   \
  })

#define str_terminate(str)                       \
  ({                                             \
    const auto str_term_tmp              = str;  \
    str_term_tmp->buf[str_term_tmp->len] = '\0'; \
  })

// `out` must be `char[N]`.
#define str_copy(out, src)                                                    \
  ({                                                                          \
    const auto tmp_str = out;                                                 \
    const auto tmp_cap = str_cap(tmp_str);                                    \
    Ind        tmp_len;                                                       \
    Err        tmp_err = str_copy_impl(tmp_str->buf, src, tmp_cap, &tmp_len); \
    if (!tmp_err) tmp_str->len = tmp_len;                                     \
    tmp_err;                                                                  \
  })

// Returns true if about to overflow.
#define str_push(str, byte)                     \
  ({                                            \
    const auto     tmp_str  = str;              \
    constexpr auto tmp_ceil = str_cap(str) - 1; \
    aver(tmp_str->len < tmp_ceil);              \
    tmp_str->buf[tmp_str->len++] = byte;        \
    tmp_str->len >= tmp_ceil;                   \
  })

typedef str_buf(128) Word_str;
typedef str_buf(256) Path_str;
