import' std:lang_s.f

\ Self-asm makes it easy to directly invoke syscalls.
\
\ x8 = 64 -> Linux write syscall.
\ x16 = 4 -> XNU write syscall.
\
\ Inspired by Cosmopolitan Libc: https://justine.lol/cosmopolitan.
: sys_write ( fd str len -- ) [
  0   27 -24 asm_load_pre      comp_instr \ ldr x0, [x27, -24]!
  1 2 27  8  asm_load_pair_off comp_instr \ ldp x1, x2, [x27, 8]
  64 8                         comp_load  \ mov x8, 64
  4 16                         comp_load  \ mov x16, 4
             asm_svc           comp_instr \ svc 666
  0          asm_zero_reg      comp_instr \ eor x0, x0, x0
] ;

: main
  1 " hello world! (using direct syscall)" sys_write
  lf
;
main
