#pragma once
#include "./err.h"
#include "./num.h"

#define str_buf(buf_len) \
  struct {               \
    Ind  len;            \
    char buf[buf_len];   \
  }

typedef str_buf(128) Word_str;
typedef str_buf(256) Path_str;

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

#define str_copy(out, src)                                                    \
  ({                                                                          \
    const auto tmp_str = out;                                                 \
    const auto tmp_cap = str_cap(tmp_str);                                    \
    Ind        tmp_len;                                                       \
    Err        tmp_err = str_copy_impl(tmp_str->buf, src, tmp_cap, &tmp_len); \
    if (!tmp_err) tmp_str->len = tmp_len;                                     \
    tmp_err;                                                                  \
  })

// Checks bounds, returns true if about to overflow, panics on overflow.
#define str_push(str, byte)                     \
  ({                                            \
    const auto     tmp_str  = str;              \
    constexpr auto tmp_ceil = str_cap(str) - 1; \
    aver(tmp_str->len < tmp_ceil);              \
    tmp_str->buf[tmp_str->len++] = byte;        \
    tmp_str->len >= tmp_ceil;                   \
  })

// // Returns 0 when out of length.
// #define str_pop(str)                                          \
//   ({                                                          \
//     const auto tmp_str = str;                                 \
//     tmp_str->len ? tmp_str->buf[tmp_str->len-- - (Ind)1] : 0; \
//   })

#define str_eq(str, tar)                             \
  ({                                                 \
    const auto str_eq_tmp = str;                     \
    !strncmp(tar, str_eq_tmp->buf, str_eq_tmp->len); \
  })

// TODO when running out of capacity, return error instead of crashing.
#define str_fmt(str, fmt, ...)                                            \
  ({                                                                      \
    const auto tmp_str = str;                                             \
    const auto tmp_len = snprintf(                                        \
      tmp_str->buf, arr_cap(tmp_str->buf), fmt __VA_OPT__(, ) __VA_ARGS__ \
    );                                                                    \
    aver((Uint)tmp_len < arr_cap(tmp_str->buf));                          \
    tmp_str->len = (Ind)tmp_len;                                          \
  })
