#pragma once

/*
TODO:
- x64 assembler (core in C, the rest in Forth)
- ability to use multiple assemblers at once
*/
#ifdef __aarch64__
#include "./c_asm_arm64.h" // IWYU pragma: export
#else
#error "unsupported CPU architecture (Arm64 only)"
#endif
