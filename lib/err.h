#pragma once
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

#define averr(expr)                                   \
  {                                                   \
    Err tmp_err = expr;                               \
    if (tmp_err) {                                    \
      fatal_expr(tmp_err, #expr, __FILE__, __LINE__); \
    }                                                 \
  }

#ifdef FAST_CRASH
#define try averr
#else
#define try(expr)                \
  {                              \
    Err tmp_err = expr;          \
    if (tmp_err) return tmp_err; \
  }
#endif

/*
For procedures which return 0/-1 and set `errno`.
Not suitable for procedures like `mmap` where the
value may be either useful, or the -1 sentinel.
*/
#define try_errno(expr) try(err_errno(expr))

#define err_errno(expr)                                                      \
  ((expr) == -1 ? err_from_errno(errno, #expr, __func__, __FILE__, __LINE__) \
                : nullptr)

#define err_direct(expr)                                                   \
  ({                                                                       \
    const auto tmp_err = expr;                                             \
    tmp_err ? err_from_errno(tmp_err, #expr, __func__, __FILE__, __LINE__) \
            : nullptr;                                                     \
  })

#define try_main_errno(expr) try_main(err_errno(expr))

#define try_main(expr)                         \
  {                                            \
    Err tmp_err = (expr);                      \
    if (tmp_err) {                             \
      fprintf(stderr, "error: %s\n", tmp_err); \
      backtrace_print();                       \
      if (DEBUG) abort();                      \
      return 1;                                \
    }                                          \
  }

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

// Blame `clang-format` for the horrible ternary style.
#define err_wrapf(fmt, err, ...)                           \
  ({                                                       \
    const auto err_wrapf_err = err;                        \
    const auto err_wrapf_buf = !err_wrapf_err ? nullptr    \
      : err_wrapf_err == ERR_BUF_0            ? ERR_BUF_1  \
                                              : ERR_BUF_0; \
    !err_wrapf_err ? nullptr : ({                          \
      snprintf(                                            \
        err_wrapf_buf,                                     \
        arr_cap(ERR_BUF_0),                                \
        fmt,                                               \
        err_wrapf_err __VA_OPT__(, ) __VA_ARGS__           \
      );                                                   \
      err_wrapf_buf;                                       \
    });                                                    \
  })

#define buf_fmtf(buf, fmt, ...)                                      \
  ({                                                                 \
    auto tmp_buf = buf;                                              \
    snprintf(tmp_buf, arr_cap(buf), fmt __VA_OPT__(, ) __VA_ARGS__); \
    tmp_buf;                                                         \
  })

#define static_assert_type(val, typ)                                     \
  static_assert(                                                         \
    __builtin_types_compatible_p(typeof_unqual(val), typeof_unqual(typ)) \
  )
