#pragma once

/*
`getpagesize()` and `sysconf(_SC_PAGESIZE)` exist,
but we define some page-aligned arrays where size
has to be hardcoded. This size should be fine for
maybe about a decade.
*/
static constexpr unsigned int MEM_PAGE = 1 << 14; // 16 KiB

#define is_aligned(ptr) __builtin_is_aligned(ptr, alignof(typeof(*ptr)))

#define ptr_clear(ptr) (*(ptr) = (typeof(*ptr)){})
