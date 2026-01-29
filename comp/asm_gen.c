#pragma once
#include "./interp.h"
#include <stddef.h>

/*
Lets us pass C struct offets to assembly code.
Similar tricks are used in the Linux kernel.

  - https://github.com/torvalds/linux/blob/master/include/linux/kbuild.h
  - https://github.com/torvalds/linux/blob/master/arch/x86/kernel/asm-offsets.c
  - https://github.com/torvalds/linux/blob/master/Kbuild
*/

/*
In `__asm__`, argument passing is not supported at the file root level,
so we have to wrap this. The definitions are still file root level.
*/
void _(void) {
  __asm__(".set INTERP_INTS_FLOOR, %0" ::"i"(INTERP_INTS_FLOOR));
  __asm__(".set INTERP_INTS_TOP, %0" ::"i"(INTERP_INTS_TOP));
}

/*
#define asm_offset(name, type, field) \
  __asm__(".set " #name ", %0" ::"i"(offsetof(type, field)))

  asm_offset(INTERP_INTS_TOP, Interp, ints.top);
*/
