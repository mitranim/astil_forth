#pragma once
#include "./err.h"
#include <inttypes.h>
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
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFFull // sizeof(size_t) == 8

typedef uint32_t Ind;
#define FMT_IND "%u"
static constexpr Ind IND_MAX = UINT32_MAX;

#elif SIZE_MAX == 0xFFFFFFFFull // sizeof(size_t) == 4

typedef uint16_t Ind;
#define FMT_IND "%u"
static constexpr Ind IND_MAX = UINT16_MAX;

#else // SIZE_MAX

typedef Uint Ind;
#define FMT_IND "%zu"
static constexpr Ind IND_MAX = SIZE_MAX;

#endif // SIZE_MAX
// clang-format on

#define FMT_UINT "%zu"
#define FMT_SINT "%zd"
#define FMT_UINT_HEX "0x%zx"

// Also see "standard" printing macros with obscure names in `inttypes.h`.
#define FMT_U8 "%d"
#define FMT_S8 "%d"
#define FMT_U16 "%d"
#define FMT_S16 "%d"
#define FMT_U32 "%u"
#define FMT_S32 "%d"
#define FMT_U64 "%llu"
#define FMT_S64 "%lld"
#define FMT_F32 "%f"
#define FMT_F64 "%f"

static constexpr auto INVALID_IND = (Ind)-1;

#define ind_valid(val)                                                    \
  ({                                                                      \
    static_assert(__builtin_types_compatible_p(typeof_unqual(val), Ind)); \
    val != INVALID_IND;                                                   \
  })

#define add_inner(tmp, one, two)                   \
  ({                                               \
    typeof_unqual(one) tmp;                        \
    aver(!__builtin_add_overflow(one, two, &tmp)); \
    tmp;                                           \
  })

#define add(...) add_inner(UNIQ_IDENT, __VA_ARGS__)

#define sub_inner(tmp, one, two)                   \
  ({                                               \
    typeof_unqual(one) tmp;                        \
    aver(!__builtin_sub_overflow(one, two, &tmp)); \
    tmp;                                           \
  })

#define sub(...) sub_inner(UNIQ_IDENT, __VA_ARGS__)

#define mul_inner(tmp, one, two)                   \
  ({                                               \
    typeof_unqual(one) tmp;                        \
    aver(!__builtin_mul_overflow(one, two, &tmp)); \
    tmp;                                           \
  })

#define mul(...) mul_inner(UNIQ_IDENT, __VA_ARGS__)

#define min_inner(tmp_A, tmp_B, A, B)               \
  ({                                                \
    const auto tmp_A = A;                           \
    const auto tmp_B = B;                           \
    (typeof(tmp_A))(tmp_A < tmp_B ? tmp_A : tmp_B); \
  })

#define min(...) min_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define max_inner(tmp_A, tmp_B, A, B)               \
  ({                                                \
    const auto tmp_A = A;                           \
    const auto tmp_B = B;                           \
    (typeof(tmp_A))(tmp_A > tmp_B ? tmp_A : tmp_B); \
  })

#define max(...) max_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

/*
For powers of 2 this is equivalent to `big % smol`
but avoids "div" instructions which may be slower.
*/
#define modulo2(big, smol) ((big) & ((smol) - 1))

#define ALLOW_OVERFLOW __attribute__((no_sanitize("integer")))

#define divide_round_up_inner(tmp, src, div) \
  ({                                         \
    const auto tmp = div;                    \
    (src + tmp - 1) / tmp;                   \
  })

#define divide_round_up(...) divide_round_up_inner(UNIQ_IDENT, __VA_ARGS__)
