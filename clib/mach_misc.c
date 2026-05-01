#pragma once
#include "./arr.h"
#include "./misc.h"
#include "./num.h"
#include <stdio.h>

static U32 mach_ver(U32 major, U32 minor, U32 patch) {
  return (major << 16u) | (minor << 8u) | patch;
}

static U16 mach_ver_major(U32 src) { return src >> 16u; }

static U16 mach_ver_minor(U32 src) { return (src & 0xFF00u) >> 8u; }

static U16 mach_ver_patch(U32 src) { return src & 0xFFu; }

/*
// `clang-tidy` complains about `sscanf`.
static U32 mach_ver_from_str(const char *src) {
  U32 major = 0;
  U32 minor = 0;
  U32 patch = 0;
  sscanf(src, "%u.%u.%u", &major, &minor, &patch);
  return mach_ver(major, minor, patch);
}
*/

static thread_local char MACH_VER_BUF[32];

// For debugging.
static char *mach_ver_to_str(U32 src) {
  snprintf(
    MACH_VER_BUF,
    arr_cap(MACH_VER_BUF),
    "%u.%u.%u",
    mach_ver_major(src),
    mach_ver_minor(src),
    mach_ver_patch(src)
  );
  return MACH_VER_BUF;
}

// int main(void) { puts(mach_ver_to_str(mach_ver(1234, 56, 78))); }
