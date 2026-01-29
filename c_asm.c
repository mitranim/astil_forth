#pragma once

#ifdef __aarch64__
#include "./c_asm_arm64.c" // IWYU pragma: export
#else
#error "unsupported CPU architecture (arm64 only)"
#endif
