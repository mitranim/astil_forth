#pragma once

#ifdef __aarch64__
#include "./arch_arm64.c" // IWYU pragma: export
#else
#error "unsupported CPU architecture (Arm64 only)"
#endif
