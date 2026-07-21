#include "./interp.h"
#include <stdio.h>

/*
Lets us pass C struct offsets to assembly code.
Similar tricks are used in the Linux kernel.

  - https://github.com/torvalds/linux/blob/master/include/linux/kbuild.h
  - https://github.com/torvalds/linux/blob/master/arch/x86/kernel/asm-offsets.c
  - https://github.com/torvalds/linux/blob/master/Kbuild

This file used to have stuff like `__asm__(".set ...")`
used with `clang -S`; we parsed the output. Then Clang
completely changed the syntax of generated statements.
Printing this ourselves is way less fragile.
*/
int main() {
  printf(".set CTX_CELLS_FLOOR, %d\n", (int)CTX_CELLS_FLOOR);
  printf(".set CTX_CELLS_TOP, %d\n", (int)CTX_CELLS_TOP);
}
