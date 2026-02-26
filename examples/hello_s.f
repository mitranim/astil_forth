import' std:lang_s.f

log" hello world! (using libc IO)" lf

\ Don't have to use libc for IO. Can just call into the kernel.
: sys_write ( fd str len -- ) [
  0   27 -24 asm_load_pre      comp_instr \ ldr x0, [x27, -24]!
  1 2 27  8  asm_load_pair_off comp_instr \ ldp x1, x2, [x27, 8]
  4 16                         comp_load  \ mov x16, 4
             asm_svc           comp_instr \ svc 666
  0          asm_zero_reg      comp_instr \ eor x0, x0, x0
] ;

flush
STDOUT " hello world! (using syscall)" sys_write lf

: bool_str if c" true" else c" false" end ;
log" bool_str(false): " false bool_str puts lf
log" bool_str(true):  " true  bool_str puts lf

64 putchar 65 putchar 66 putchar 67 putchar lf \ @ABC
