import' ./lang_s.f

log" hello world! (top level)" lf

: hello log" hello world! (deferred)" lf ;
hello

(
Don't have to use libc for IO.
Can just call into the kernel.

x8 = 64 -> Linux write syscall.
x16 = 4 -> XNU write syscall.

Inspired by Cosmopolitan Libc: https://justine.lol/cosmopolitan.
However, our system relies on some XNU-specific stuff elsewhere.
)
: sys_write ( fd str len -- ) [
  0   27 -24 asm_load_pre      comp_instr \ ldr x0, [x27, -24]!
  1 2 27  8  asm_load_pair_off comp_instr \ ldp x1, x2, [x27, 8]
  64 8                         comp_load  \ mov x8, 64
  4 16                         comp_load  \ mov x16, 4
             asm_svc           comp_instr \ svc 666
  0          asm_zero_reg      comp_instr \ eor x0, x0, x0
] ;

: hello0 1 " hello world! (using syscall)" sys_write lf ;
hello0

64 putchar 65 putchar 66 putchar 67 putchar lf \ @ABC

1234567890 .

: truth_num if 10 else 20 end ;
0 truth_num .
1 truth_num .

: truth_str if " true!" else " false!" end ;
0 truth_str type lf
1 truth_str type lf
