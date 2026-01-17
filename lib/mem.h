#pragma once

/*
`sysconf(_SC_PAGESIZE)` exists, but we define some
page-aligned arrays where size has to be hardcoded.
This should be fine for about a decade maybe.
*/
static constexpr unsigned int MEM_PAGE = 1 << 14; // 16 KiB

#define is_aligned(ptr) __builtin_is_aligned(ptr, alignof(typeof(*ptr)))

#define ptr_clear(ptr) (*(ptr) = (typeof(*ptr)){})
