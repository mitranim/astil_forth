#pragma once
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool DEBUG = false;

#ifdef PROD
#define IF_DEBUG(code)
#else
#define IF_DEBUG(code) \
  if (DEBUG) code;
#endif

typedef void(Void_fun)(void);

#define USED __attribute((used))

/*
Usage:

  defer(type_deinit) type name = {};
*/
#define defer(fun) __attribute__((cleanup(fun)))

// For use in `X_deinit` functions used with `defer`.
#define var_deinit(name, fun) \
  {                           \
    if (!name) return;        \
    void *tmp = *name;        \
    if (!tmp) return;         \
    *name = nullptr;          \
    fun(tmp);                 \
  }

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

// Non-lazy "or" which doesn't convert operands to 0 or 1.
#define either(one, two)         \
  ({                             \
    const auto tmp_one = one;    \
    const auto tmp_two = two;    \
    tmp_one ? tmp_one : tmp_two; \
  })

#define assign_cast(tar, src) *(tar) = (typeof(*(tar)))(src)
