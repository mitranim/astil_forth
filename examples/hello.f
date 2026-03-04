import' std:lang.f

" hello world! (using libc IO)" log lf

\ Don't have to use libc for IO. Can just call into the kernel.
: sys_write { fd str len } [
  16 4 asm_mov_imm comp_instr \ mov x16, 4
       asm_svc     comp_instr \ svc 666
] ;

flush
STDOUT s" hello world! (using syscall)" sys_write lf

: bool_str { val -- str }
  val
  if " true" ret end
  " false"
;

" bool_str(false): " log false bool_str log lf
" bool_str(true):  " log true  bool_str log lf

char' @ logc
char' A logc
char' B logc
char' C logc
lf \ @ABC
