import' std:lang.f

log" hello world! (using libc IO)" lf

\ Don't have to use libc for IO. Can just call into the kernel.
: sys_write { fd str len } [
  16 4 asm_mov_imm comp_instr \ mov x16, 4
       asm_svc     comp_instr \ svc 666
] ;

flush
STDOUT " hello world! (using syscall)" sys_write lf

: bool_str { val -- str }
  val if c" true" ret end
  c" false"
;

log" bool_str(false): " false bool_str puts lf
log" bool_str(true):  " true  bool_str puts lf

64 putchar 65 putchar 66 putchar 67 putchar lf \ @ABC
