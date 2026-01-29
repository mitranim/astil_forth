#pragma once
#include "./misc.h"
#include "./num.h"
#include <stdio.h>

static U32 mach_ver(U32 major, U32 minor, U32 patch) {
  return major << 16 | minor << 8 | patch;
}

static U32 mach_ver_from_str(const char *src) {
  U32 major = 0;
  U32 minor = 0;
  U32 patch = 0;
  sscanf(src, "%u.%u.%u", &major, &minor, &patch);
  return mach_ver(major, minor, patch);
}

static thread_local char MACH_VER_BUF[32];

// For debugging.
static char *mach_ver_to_str(U32 src) {
  snprintf(
    MACH_VER_BUF,
    arr_cap(MACH_VER_BUF),
    "%u.%u.%u",
    ((src >> 16) & 0xFFFF),
    ((src >> 8) & 0xFF),
    (src & 0xFF)
  );
  return MACH_VER_BUF;
}

// int main(void) { puts(mach_ver_to_str(mach_ver(1234, 56, 78))); }
