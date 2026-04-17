#pragma once
#include "./num.h"
#include <string.h>

/*
`getpagesize()` and `sysconf(_SC_PAGESIZE)` exist,
but we define some page-aligned arrays where size
has to be hardcoded. This size should be fine for
maybe about a decade.
*/
static constexpr Ind MEM_PAGE = 1 << 14; // 16 KiB

#define is_aligned_to(num, size) __builtin_is_aligned(num, size)

#define is_aligned(ptr) is_aligned_to(ptr, alignof(typeof(*ptr)))

#define ptr_clear(ptr) (*(ptr) = (typeof(*ptr)){})

#define ptr_set(tar, ...) *tar = (typeof(*tar))__VA_ARGS__

#define mem_align_page(val) __builtin_align_up(val, MEM_PAGE)

/*
Resizable buffer. Same as `list_of(U8)`; see `./list.h` and `./list.c`.
Comes with additional procedures and macros for appending binary data.
Must be zero-initialized before first use.
*/
typedef struct {
  U8 *dat;
  Ind len;
  Ind cap;
} Buf;

/*
Appends an arbitrary value to a `Buf` as raw bytes, resizing as needed.
If the buffer memory is later reinterpreted as values larger than `U8`,
the caller is responsible for address alignment.

Uses `memcpy` rather than pointer assignment to avoid UB.
*/
#define buf_append(buf, ...)                             \
  ({                                                     \
    const auto tmp_buf = buf;                            \
    const auto tmp_val = __VA_ARGS__;                    \
    buf_reserve(tmp_buf, sizeof(tmp_val));               \
    memcpy(buf_top(tmp_buf), &tmp_val, sizeof(tmp_val)); \
    (void)(tmp_buf->len += sizeof(tmp_val));             \
  })

/*
Usage:

  const auto val = buf_load(buf, some_off, Some_struct);
  val.field0 += 123;
  buf_store(buf, some_off, val);

Uses `memcpy` to avoid UB from misalignment or pointer misinterpretation.
This is also why we load/store values instead of operating on addresses.

UNTESTED.
*/
#define buf_load(buf, off, type)                           \
  ({                                                       \
    const auto tmp_buf = buf;                              \
    type       tmp_val;                                    \
    memcpy(&tmp_val, tmp_buf->dat + off, sizeof(tmp_val)); \
    tmp_val;                                               \
  })

// UNTESTED.
#define buf_store(buf, off, ...)                       \
  ({                                                   \
    const auto tmp_val = __VA_ARGS__;                  \
    memcpy(buf->dat + off, &tmp_val, sizeof(tmp_val)); \
  })
