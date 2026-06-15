#pragma once
#include "./err.h"
#include "./num.h"
#include <string.h>

/*
A fixed-size buffer for a null-terminated string.
`buf_len` includes space for the null terminator.
Field `.len` is a cached length without the null.
State contract: `len < buf_len && buf[len] == 0`.
*/
#define str_buf(buf_len) \
  struct {               \
    Ind  len;            \
    char buf[buf_len];   \
  }

// Same as `buf_len` given to the type.
#define str_cap(str) ((Ind)(sizeof((str)->buf)))

#define str_trunc_inner(tmp, str) \
  ({                              \
    const auto tmp = str;         \
    tmp->len       = 0;           \
    str_terminate(tmp);           \
  })

#define str_trunc(...) str_trunc_inner(UNIQ_IDENT, __VA_ARGS__)

#define str_terminate_inner(tmp, str)      \
  ({                                       \
    const auto tmp = str;                  \
    assert_fatal(tmp->len < str_cap(tmp)); \
    tmp->buf[tmp->len] = '\0';             \
  })

#define str_terminate(...) str_terminate_inner(UNIQ_IDENT, __VA_ARGS__)

// The source string must be null-terminated.
#define str_set_inner(tmp, out, src)                      \
  ({                                                      \
    const auto tmp = out;                                 \
    str_set_impl(tmp->buf, &tmp->len, src, str_cap(tmp)); \
  })

#define str_set(...) str_set_inner(UNIQ_IDENT, __VA_ARGS__)

#define str_push_inner(tmp_str, tmp_ceil, str, byte) \
  ({                                                 \
    const auto     tmp_str  = str;                   \
    constexpr auto tmp_ceil = str_cap(str) - 1;      \
    assert_fatal(tmp_str->len < tmp_ceil);           \
    tmp_str->buf[tmp_str->len++] = byte;             \
    tmp_str->buf[tmp_str->len]   = '\0';             \
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

#define str_eq(str, tar)                                \
  ({                                                    \
    const auto str_eq_tmp = str;                        \
    str_eq_impl(str_eq_tmp->buf, str_eq_tmp->len, tar); \
  })

#define str_fmt_inner(tmp_str, tmp_len, str, fmt, ...)                   \
  ({                                                                     \
    const auto tmp_str = str;                                            \
    const auto tmp_len = snprintf(                                       \
      tmp_str->buf, sizeof(tmp_str->buf), fmt __VA_OPT__(, ) __VA_ARGS__ \
    );                                                                   \
    assert_fatal(tmp_len >= 0);                                          \
    assert_fatal((Uint)tmp_len < sizeof(tmp_str->buf));                  \
    tmp_str->len = (Ind)tmp_len;                                         \
  })

// TODO when running out of capacity, return error instead of crashing.
#define str_fmt(...) str_fmt_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

// For literal string prefixes; skips `strlen`.
#define str_has_prefix(str, pre) (!strncmp(str, pre, (sizeof(pre) - 1)))

/*
static bool str_has_prefix(const char *str, const char *pre) {
  return !strncmp(str, pre, strlen(pre));
}
*/
