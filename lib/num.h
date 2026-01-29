#pragma once
#include <stdint.h>
#include <unistd.h>

typedef size_t  Uint;
typedef ssize_t Sint;

typedef uint8_t U8;
typedef int8_t  S8;

typedef uint16_t U16;
typedef int16_t  S16;

typedef uint32_t U32;
typedef int32_t  S32;

typedef uint64_t U64;
typedef int64_t  S64;

typedef float  F32;
typedef double F64;

/*
Unsigned index smaller than a pointer. Unlike a signed index or a pointer-sized
unsigned index, when this value underflows, adding it to a pointer produces a
significantly larger garbage address, making it more likely that the program
crashes instead of accessing unrelated memory.

Suggested by Eskil: https://www.youtube.com/watch?v=sfrnU3-EpPI&t=2517
*/
// clang-format off
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFFull
  typedef uint32_t Ind;
  static constexpr Uint IND_MAX = UINT32_MAX;
  #define FMT_IND "%u"
#elif SIZE_MAX == 0xFFFFFFFFull
  typedef uint16_t Ind;
  static constexpr Uint IND_MAX = UINT16_MAX;
  #define FMT_IND "%u"
#else
  typedef Uint Ind;
  static constexpr Uint IND_MAX = SIZE_MAX;
  #define FMT_IND "%zu"
#endif // SIZE_MAX
// clang-format on

#define FMT_UINT "%zu"
#define FMT_SINT "%zd"
#define FMT_UINT_HEX "0x%zx"

static constexpr auto INVALID_IND = (Ind)-1;

#define ind_valid(val)                                             \
  ({                                                               \
    static_assert(__builtin_types_compatible_p(typeof(val), Ind)); \
    (val) != INVALID_IND;                                          \
  })

#define max(one, two)                      \
  ({                                       \
    const auto tmp_one = one;              \
    const auto tmp_two = two;              \
    tmp_one > tmp_two ? tmp_one : tmp_two; \
  })

/*
For powers of 2 this is equivalent to `big % smol`
but avoids "div" instructions which may be slower.
*/
#define modulo2(big, smol) ((big) & ((smol) - 1))
