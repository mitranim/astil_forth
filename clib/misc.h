#pragma once
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool DEBUG = false;
static bool TRACE = false;

#ifdef PROD
#define IF_DEBUG(code)
#else
#define IF_DEBUG(code) \
  if (DEBUG) code;
#endif

#define CAT_IDENT(A, B) A##B
#define XCAT_IDENT(A, B) CAT_IDENT(A, B)
#define UNIQ_IDENT XCAT_IDENT(__uniq_, __COUNTER__)

typedef void(Void_fun)(void);

#define USED __attribute((used))

// Requires Clang 22.1 or higher and `-fdefer-ts`.
#if !defined(defer) && defined(__is_identifier) && !__is_identifier(_Defer)
#define defer _Defer
#endif

/*
Alternative to the "real" `defer` which, at the time of writing,
requires very recent compiler versions.

Usage:

  deferred(type_deinit) type name = {};

Many cleanup functions, such as `list_deinit`, are idempotent,
and may be called "manually" in addition to the automatic call
if earlier cleanup is desired. Idempotency requires sentinel
values, typically 0 / nil, but sometimes a type-specific value
such as `-1` for file descriptors. This means that variables
used with `deferred` must always be initialized with the right
sentinel value for their type; usually `{}`.
*/
#define deferred(fun) __attribute__((cleanup(fun)))

// For use in `X_deinit` functions used with `deferred`.
#define var_deinit_inner(tmp_ptr, tmp_val, name, fun) \
  {                                                   \
    const auto tmp_ptr = name;                        \
    if (!tmp_ptr) return;                             \
    void *tmp_val = *tmp_ptr;                         \
    if (!tmp_val) return;                             \
    *tmp_ptr = nullptr;                               \
    fun(tmp_val);                                     \
  }

#define var_deinit(...) var_deinit_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#ifndef unreachable
#define unreachable __builtin_unreachable
#endif // unreachable

#define cast(typ, src) (*(typ *)(src))
#define divisible_by(one, two) (!((one) % (two)))

#define range(type, name, max)  \
  type name = 0, tmp_max = max; \
  name < tmp_max;               \
  name++

#define span(max) range(auto, tmp_ind, max)

#define either_inner(tmp, A, B) \
  ({                            \
    const auto tmp = A;         \
    tmp ? tmp : B;              \
  })

// Non-lazy "or" which doesn't convert operands to 0 or 1.
#define either(...) either_inner(UNIQ_IDENT, __VA_ARGS__)

#define assign_cast(tar, src) *(tar) = (typeof(*(tar)))(src)

#define sizeof_field(type, name) sizeof(((type *)nullptr)->name)
