#pragma once

#ifndef __aarch64__
#error "unsupported CPU architecture (Arm64 only)"
#else // __aarch64__

#ifdef NATIVE_CALL_ABI
#include "./c_arch_arm64_native.c" // IWYU pragma: export
#else
#include "./c_arch_arm64_stack.c" // IWYU pragma: export
#endif

#endif // __aarch64__
