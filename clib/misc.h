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

/*
Usage:

  defer(type_deinit) type name = {};

Variables used with `defer` must always be initialized.
Typically this means zero-initialization, but sometimes
the sentinel value is type-specific, like `-1`.

The type-specific cleanup functions rely on initial
sentinel values to avoid attempting to free resources
which weren't actually allocated.
*/
#define defer(fun) __attribute__((cleanup(fun)))

/*
Usage:

  static void deinit_global_state(void*) {}
  defer_void(deinit_global_state);
*/
#define defer_void(fun) defer(fun) void *UNIQ_IDENT

// For use in `X_deinit` functions used with `defer`.
#define var_deinit_impl(tmp_ptr, tmp_val, name, fun) \
  {                                                  \
    const auto tmp_ptr = name;                       \
    if (!tmp_ptr) return;                            \
    void *tmp_val = *tmp_ptr;                        \
    if (!tmp_val) return;                            \
    *tmp_ptr = nullptr;                              \
    fun(tmp_val);                                    \
  }

#define var_deinit(...) var_deinit_impl(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

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

#define either_impl(tmp, A, B) \
  ({                           \
    const auto tmp = A;        \
    tmp ? tmp : B;             \
  })

// Non-lazy "or" which doesn't convert operands to 0 or 1.
#define either(...) either_impl(UNIQ_IDENT, __VA_ARGS__)

#define assign_cast(tar, src) *(tar) = (typeof(*(tar)))(src)

#define sizeof_field(type, name) sizeof(((type *)nullptr)->name)
