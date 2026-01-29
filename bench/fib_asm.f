import' ../forth/lang_r.f

: fib { src -- out } [
  0 1        asm_cmp_imm     comp_instr \ cmp x0, 1
  12  ASM_GT asm_branch_cond comp_instr \ b.gt <init>
  0 1        asm_mov_imm     comp_instr \ mov x0, 1
             asm_ret         comp_instr \ ret
  2 0        asm_mov_reg     comp_instr \ mov x2, x0 -- <init>
  1 1        asm_mov_imm     comp_instr \ mov x1, 1
  0 1        asm_mov_imm     comp_instr \ mov x0, 1
  3 1        asm_mov_reg     comp_instr \ mov x3, x1 -- <loop>
  1 0        asm_mov_reg     comp_instr \ mov x1, x0
  0 0 3      asm_add_reg     comp_instr \ add x0, x0, x3
  2 2 1      asm_sub_imm     comp_instr \ sub x2, x2, 1
  2 1        asm_cmp_imm     comp_instr \ cmp x2, 1
  -20 ASM_GT asm_branch_cond comp_instr \ b.gt <loop>
  1 comp_args_set
] ;

: run { count -- }
  [
    9 0 asm_mov_reg comp_instr \ mov x9, x0
  ]
  91  \ mov x0, 91 -- <loop_beg>
  fib \ bl <off>
  { -- }
  [
    9 9 1 asm_sub_imm             comp_instr \ sub x9, x9, 1
    9 -12 asm_cmp_branch_not_zero comp_instr \ cbnz x9, <loop_beg>
  ]
;

: main
  1 16 lsl run
  \ log" fib(91): " 91 fib log_int lf
;
main
