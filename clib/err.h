#pragma once
#include "./misc.h"
#include <errno.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MUST_USE __attribute((warn_unused_result))

MUST_USE typedef const char *Err;

typedef Err(Err_fun)(void);

// Assert with backtrace. Name yoinked from SBCL.
#define aver(expr)                                                           \
  ({                                                                         \
    static_assert(                                                           \
      !__builtin_types_compatible_p(typeof_unqual(expr), typeof_unqual(Err)) \
    );                                                                       \
    if (!(expr)) aver_fatal(#expr, __FILE__, __LINE__);                      \
  })

#define averr_inner(tmp, expr)                    \
  {                                               \
    Err tmp = expr;                               \
    if (tmp) {                                    \
      fatal_expr(tmp, #expr, __FILE__, __LINE__); \
    }                                             \
  }

#define averr(...) averr_inner(UNIQ_IDENT, __VA_ARGS__)

#ifdef FAST_CRASH
#define try averr
#else
#define try_inner(tmp, expr) \
  {                          \
    Err tmp = expr;          \
    if (tmp) return tmp;     \
  }
#define try(...) try_inner(UNIQ_IDENT, __VA_ARGS__)
#endif

/*
For procedures which return 0/-1 and set `errno`.

Discards the value in case of non-error, which makes this unsuitable
for `mmap`, whose result is either -1 or an address.
*/
#define try_errno(expr) try(err_errno(expr))

#define err_errno(expr)                                                      \
  ((expr) == -1 ? err_from_errno(errno, #expr, __func__, __FILE__, __LINE__) \
                : nullptr)

// For procedures which directly return errno or 0.
#define try_errno_posix(expr) try(err_errno_posix(expr))

#define err_errno_posix_inner(tmp, expr)                                      \
  ({                                                                          \
    const auto tmp = expr;                                                    \
    tmp ? err_from_errno(tmp, #expr, __func__, __FILE__, __LINE__) : nullptr; \
  })

#define err_errno_posix(...) err_errno_posix_inner(UNIQ_IDENT, __VA_ARGS__)

#define try_main_errno(expr) try_main(err_errno(expr))

#define try_main_inner(tmp, expr)          \
  {                                        \
    Err tmp = (expr);                      \
    if (tmp) {                             \
      fprintf(stderr, "error: %s\n", tmp); \
      if (TRACE) backtrace_print();        \
      if (DEBUG) abort();                  \
      return 1;                            \
    }                                      \
  }

#define try_main(...) try_main_inner(UNIQ_IDENT, __VA_ARGS__)

#ifdef NO_TRACE
#define err_str(val) val
#else
#define err_str(val) (backtrace_capture(), val)
#endif // NO_TRACE

#ifdef FAST_CRASH
#define errf(fmt, ...)                                                        \
  ({                                                                          \
    fatal(                                                                    \
      buf_fmtf(ERR_BUF_0, fmt __VA_OPT__(, ) __VA_ARGS__), __FILE__, __LINE__ \
    );                                                                        \
    (Err) nullptr;                                                            \
  })
#elifdef NO_TRACE
#define errf(fmt, ...) buf_fmtf(ERR_BUF_0, fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define errf(fmt, ...)                                   \
  ({                                                     \
    backtrace_capture();                                 \
    buf_fmtf(ERR_BUF_0, fmt __VA_OPT__(, ) __VA_ARGS__); \
  })
#endif // FAST_CRASH NO_TRACE

#ifndef arr_cap
#define arr_cap(val) (sizeof(val) / sizeof(val[0]))
#endif

// Blame `clang-format` for the horrible ternary fmting.
#define err_wrapf_inner(tmp_err, tmp_buf, fmt, err, ...)                     \
  ({                                                                         \
    const auto tmp_err = err;                                                \
    const auto tmp_buf = !tmp_err ? nullptr                                  \
      : tmp_err == ERR_BUF_0      ? ERR_BUF_1                                \
                                  : ERR_BUF_0;                               \
    !tmp_err ? nullptr : ({                                                  \
      snprintf(                                                              \
        tmp_buf, arr_cap(ERR_BUF_0), fmt, tmp_err __VA_OPT__(, ) __VA_ARGS__ \
      );                                                                     \
      tmp_buf;                                                               \
    });                                                                      \
  })

#define err_wrapf(...) err_wrapf_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define buf_fmtf_inner(tmp, buf, fmt, ...)                       \
  ({                                                             \
    auto tmp = buf;                                              \
    snprintf(tmp, arr_cap(buf), fmt __VA_OPT__(, ) __VA_ARGS__); \
    tmp;                                                         \
  })

#define buf_fmtf(...) buf_fmtf_inner(UNIQ_IDENT, __VA_ARGS__)

#define static_assert_type(val, typ)                                     \
  static_assert(                                                         \
    __builtin_types_compatible_p(typeof_unqual(val), typeof_unqual(typ)) \
  )

// For error messages.
static char thread_local ERR_BUF_0[4096];
static char thread_local ERR_BUF_1[4096];

// For backtraces.
static thread_local void *BT_BUF[256] = {};
static thread_local int   BT_BUF_LEN  = 0;
