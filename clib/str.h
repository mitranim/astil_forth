#pragma once
#include "./err.h"
#include "./num.h"
#include <string.h>

#define str_buf(buf_len) \
  struct {               \
    Ind  len;            \
    char buf[buf_len];   \
  }

// SYNC[word_str].
typedef str_buf(128) Word_str;

#define str_cap(str) ((Ind)(sizeof((str)->buf) / sizeof((str)->buf[0])))

#define str_trunc_inner(tmp, str) \
  ({                              \
    const auto tmp = str;         \
    tmp->len       = 0;           \
    str_terminate(tmp);           \
  })

#define str_trunc(...) str_trunc_inner(UNIQ_IDENT, __VA_ARGS__)

#define str_terminate_inner(tmp, str) \
  ({                                  \
    const auto tmp     = str;         \
    tmp->buf[tmp->len] = '\0';        \
  })

#define str_terminate(...) str_terminate_inner(UNIQ_IDENT, __VA_ARGS__)

#define str_set_inner(tmp_str, tmp_len, out, src)                              \
  ({                                                                           \
    const auto tmp_str = out;                                                  \
    Ind        tmp_len;                                                        \
    Err tmp_err = str_set_impl(tmp_str->buf, src, str_cap(tmp_str), &tmp_len); \
    if (!tmp_err) tmp_str->len = tmp_len;                                      \
    tmp_err;                                                                   \
  })

/*
Uses `strlcpy`. The source string must be null-terminated.
The capacity of the source buffer is allowed to be smaller
or larger than of the destination buffer.
*/
#define str_set(...) str_set_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define str_push_inner(tmp_str, tmp_ceil, str, byte) \
  ({                                                 \
    const auto     tmp_str  = str;                   \
    constexpr auto tmp_ceil = str_cap(str) - 1;      \
    aver(tmp_str->len < tmp_ceil);                   \
    tmp_str->buf[tmp_str->len++] = byte;             \
    tmp_str->len >= tmp_ceil;                        \
  })

// Checks bounds, returns true if about to overflow, panics on overflow.
#define str_push(...) str_push_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

// // Returns 0 when out of length.
// #define str_pop(str)                                          \
//   ({                                                          \
//     const auto tmp_str = str;                                 \
//     tmp_str->len ? tmp_str->buf[tmp_str->len-- - (Ind)1] : 0; \
//   })

#define str_eq_inner(tmp, str, tar)    \
  ({                                   \
    const auto tmp = str;              \
    !strncmp(tar, tmp->buf, tmp->len); \
  })

#define str_eq(...) str_eq_inner(UNIQ_IDENT, __VA_ARGS__)

#define str_fmt_inner(tmp_str, tmp_len, str, fmt, ...)                    \
  ({                                                                      \
    const auto tmp_str = str;                                             \
    const auto tmp_len = snprintf(                                        \
      tmp_str->buf, arr_cap(tmp_str->buf), fmt __VA_OPT__(, ) __VA_ARGS__ \
    );                                                                    \
    aver((Uint)tmp_len < arr_cap(tmp_str->buf));                          \
    tmp_str->len = (Ind)tmp_len;                                          \
  })

// TODO when running out of capacity, return error instead of crashing.
#define str_fmt(...) str_fmt_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

// For literal string prefixes; skips `strlen`.
#define str_has_prefix(str, pre) (!strncmp(str, pre, (arr_cap(pre) - 1)))

/*
static bool str_has_prefix(const char *str, const char *pre) {
  return !strncmp(str, pre, strlen(pre));
}
*/
