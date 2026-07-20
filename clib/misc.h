#pragma once
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool DEBUG = false;
static bool TRACE = false;

#ifdef NDEBUG
#define IF_DEBUG(code)
#else
#define IF_DEBUG(code) \
  if (DEBUG) code;
#endif

#define CAT_IDENT(A, B) A##B
#define XCAT_IDENT(A, B) CAT_IDENT(A, B)
#define UNIQ_IDENT XCAT_IDENT(__uniq_, __COUNTER__)

typedef void Void_fun(void);

#define USED __attribute((used))

/*
Limited alternative to the "real defer" which, at the time of writing,
requires very recent versions of Clang or GCC.

"Real defer" looks like this:

  #include <stddefer.h>
  type name = {};
  defer type_deinit(&name);

This alternative is used like this:

  deferred(type_deinit) type name = {};

Many cleanup functions, such as `list_deinit`, are idempotent,
and may be called "manually" in addition to the automatic call
if earlier cleanup is desired. Idempotency requires sentinel
values, typically 0 / nil, but sometimes a type-specific value
such as `-1` for file descriptors. This means that variables
used with `deferred` must always be initialized with the right
sentinel value for their type; usually `{}`.

General lifecycle contract for complex C values:

- `Type_init(&val)` initializes undefined storage and clears `val` first.
- `Type_init` may leave `val` partially initialized on error.
- Callers must defer cleanup: `deferred(Type_deinit) Type val = {};`.
- `Type_deinit(&val)` must tolerate zero and partially-inited values,
  perform best-effort cleanup, and leave `val` in its sentinel state.
- Calling `Type_init` on a live value without deiniting is a bug.

Simple values without `Type_init` rely on caller-provided sentinel init.
Many structures, such as stack / list / dict, are valid after zero-init.
Some values, such as file descriptors, require special sentinel like -1.
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

#define either_inner(tmp_A, tmp_B, A, B) \
  ({                                     \
    const auto tmp_A = A;                \
    const auto tmp_B = B;                \
    tmp_A ? tmp_A : tmp_B;               \
  })

// Non-lazy "or" which doesn't convert operands to 0 or 1.
#define either(...) either_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define assign_cast(tar, src) *(tar) = (typeof(*(tar)))(src)

#define sizeof_field(type, name) (Ind)sizeof(((type *)nullptr)->name)
