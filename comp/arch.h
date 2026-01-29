#pragma once

/*
TODO:
- x64 assembler (core in C, the rest in Forth)
- Ability to use multiple assemblers at once.
- Cross-compilation.
*/

#ifdef __aarch64__
#include "./arch_arm64.h" // IWYU pragma: export
#else
#error "unsupported CPU architecture (Arm64 only)"
#endif
