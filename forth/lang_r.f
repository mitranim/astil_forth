:: \ 10 parse { -- } ;
:: ( 41 parse { -- } ;

\ Can use comments now!
\
\ This file boostraps the Forth language. The outer interpreter / compiler
\ provides only the most fundamental intrinsics; enough for self-assembly.
\ We define basic words in terms of machine instructions, building up from
\ there. Currently only the Arm64 CPU architecture is supported.
\
\ The compiler and language come in two variants: stack-CC and register-CC.
\ This file uses the native register-based calling convention. See adjacent
\ file `./lang_s.f` for the stack-based calling convention.

\ brk 666
: abort [ 0b110_101_00_001_0000001010011010_000_00 comp_instr ] ;
: unreachable abort ;
: nop ;

\ ## Compilation-related words
\
\ These words are used for metaprogramming and mostly rely on
\ compiler intrinsics. We define them first to be able to use
\ constants via `let:`.
\
\ Unlike in standard Forth, we use separate wordlists for
\ regular and compile-time / immediate words. As a result,
\ we have to define parsing words like "compile" in pairs:
\ one tick seeks in the regular wordlist, and double tick
\ seeks in the compile-time wordlist.

\ Non-immediate replacement for standard `literal`.
: comp_push { val } ( E: -- val ) comp_next_arg_reg { reg } val reg comp_load ;

: next_word { list -- XT } ( "word" -- XT ) parse_word list find_word ;

:  tick_next { list } ( "word" -- ) ( E: -- XT ) list next_word comp_push ;
:  '  {    -- XT } (    "word" -- ) 1 next_word ; \ WORDLIST_EXEC
:  '' {    -- XT } (    "word" -- ) 2 next_word ; \ WORDLIST_COMP
:: '  ( E: -- XT ) ( C: "word" -- ) 1 tick_next ; \ WORDLIST_EXEC
:: '' ( E: -- XT ) ( C: "word" -- ) 2 tick_next ; \ WORDLIST_COMP

:  inline_next { list } ( "word" -- ) ( E: <word> )
  list next_word inline_word
;
:: inline'  1 inline_next ;
:: inline'' 2 inline_next ;

\ "execute" is renamed from standard "postpone".
:  execute_next { list } ( "word" -- ) ( E: <word> )
  list next_word comp_call
;
:: execute'  1 execute_next ;
:: execute'' 2 execute_next ;

:  compile_next { list } ( "word" -- )
  list next_word comp_push ' comp_call comp_call
;
:: compile'  1 compile_next ;
:: compile'' 2 compile_next ;

: semicolon execute'' ; ;

: let_init { str len val } ( C: "name" -- ) ( E: -- val )
  \ Execution-time semantics.
  str len colon
    0 1 comp_signature ( E: -- val )
    val comp_push
  semicolon

  \ Compile-time semantics.
  str len colon_colon
    val comp_push compile' comp_push
  semicolon
;

\ Similar to standard `constant`.
: let: { val } ( C: "name" -- ) ( E: -- val )
  parse_word val let_init
  [ not_comp_only ]
;

0 let: nil
0 let: false
1 let: true

1 let: WORDLIST_EXEC
2 let: WORDLIST_COMP

\ ## Assembler and bitwise ops
\
\ We interleave definitions of assembler words with definitions
\ of various arithmetic words used by the assembler.

4      let: ASM_INSTR_SIZE
8      let: ASM_REG_SCRATCH
27     let: ASM_REG_INTERP
28     let: ASM_REG_ERR
29     let: ASM_REG_FP
30     let: ASM_REG_LR
31     let: ASM_REG_SP
0b0000 let: ASM_EQ
0b0001 let: ASM_NE
0b0010 let: ASM_CS
0b0011 let: ASM_CC
0b0100 let: ASM_MI
0b0101 let: ASM_PL
0b0110 let: ASM_VS
0b0111 let: ASM_VC
0b1000 let: ASM_HI
0b1001 let: ASM_LS
0b1010 let: ASM_GE
0b1011 let: ASM_LT
0b1100 let: ASM_GT
0b1101 let: ASM_LE
0b1110 let: ASM_AL
0b1111 let: ASM_NV
666    let: ASM_PLACEHOLDER

: or { i0 i1 -- i2 } [
  0b1_01_01010_00_0_00001_000000_00000_00000 comp_instr \ orr x0, x0, x1
  1 comp_args_set
] ;

: and { i0 i1 -- i2 } [
  0b1_00_01010_00_0_00001_000000_00000_00000 comp_instr \ and x0, x0, x1
  1 comp_args_set
] ;

: xor { i0 i1 -- i2 } [
  0b1_10_01010_00_0_00001_000000_00000_00000 comp_instr \ eor x0, x0, x1
  1 comp_args_set
] ;

: lsl { i0 bits -- i1 } [
  0b1_0_0_11010110_00001_0010_00_00000_00000 comp_instr \ lsl x0, x0, x1
  1 comp_args_set
] ;

: lsr { i0 bits -- i1 } [
  0b1_0_0_11010110_00001_0010_01_00000_00000 comp_instr \ lsr x0, x0, x1
  1 comp_args_set
] ;

: invert { i0 -- i1 } [
  0b1_01_01010_00_1_00000_000000_11111_00000 comp_instr \ mvn x0, x0
  1 comp_args_set
] ;

\ Our parsing rules prevent `1+` or `+1` from being a word name.
: inc { i0 -- i1 } [
  0b1_0_0_100010_0_000000000001_00000_00000 comp_instr \ add x0, x0, 1
  1 comp_args_set
] ;

\ Our parsing rules prevent `1-` or `-1` from being a word name.
: dec { i0 -- i1 } [
  0b1_1_0_100010_0_000000000001_00000_00000 comp_instr \ sub x0, x0, 1
  1 comp_args_set
] ;

: low_bits { len -- bits } 1 len lsl dec ;
: bit_trunc { imm len -- imm } len low_bits imm and ;

\ cmp Xn, <imm>
: asm_cmp_imm { Xn imm -- instr }
  imm  10 lsl { imm }
  Xn   5  lsl
  imm or
  0b1_1_1_100010_0_000000000000_00000_11111 or
;

\ cmp Xn, 0
: asm_cmp_zero { Xn -- instr } Xn 0 asm_cmp_imm ;

\ cset Xd, <cond>
: asm_cset { Xd cond -- instr }
  cond 1 xor 12 lsl { cond }
  Xd cond or
  0b1_0_0_11010100_11111_0000_0_1_11111_00000 or
;

: <>0 { int -- bool } [
  0        asm_cmp_zero comp_instr \ cmp x0, 0
  0 ASM_NE asm_cset     comp_instr \ cset x0, ne
  1                     comp_args_set
] ;

: =0 { int -- bool } [
  0        asm_cmp_zero comp_instr \ cmp x0, 0
  0 ASM_EQ asm_cset     comp_instr \ cset x0, eq
  1                     comp_args_set
] ;

\ Shared by a lot of load and store instructions. 64-bit only.
: asm_pattern_load_store_pair { Xt1 Xt2 Xn imm7 -- pattern }
  Xt2                    10 lsl { Xt2 }
  Xn                      5 lsl { Xn }
  imm7 3 lsr 7 bit_trunc 15 lsl
  Xt1  or
  Xt2  or
  Xn   or
;

\ ldp Xt1, Xt2, [Xn, <imm>]!
: asm_load_pair_pre { Xt1 Xt2 Xn imm7 -- instr }
  Xt1 Xt2 Xn imm7 asm_pattern_load_store_pair
  0b10_101_0_011_1_0000000_00000_00000_00000 or
;

\ ldp Xt1, Xt2, [Xn], <imm>
: asm_load_pair_post { Xt1 Xt2 Xn imm7 -- instr }
  Xt1 Xt2 Xn imm7 asm_pattern_load_store_pair
  0b10_101_0_001_1_0000000_00000_00000_00000 or
;

\ ldp Xt1, Xt2, [Xn, <imm>]
: asm_load_pair_off { Xt1 Xt2 Xn imm7 -- instr }
  Xt1 Xt2 Xn imm7 asm_pattern_load_store_pair
  0b10_101_0_010_1_0000000_00000_00000_00000 or
;

\ stp Xt1, Xt2, [Xn, <imm>]!
: asm_store_pair_pre { Xt1 Xt2 Xn imm7 -- instr }
  Xt1 Xt2 Xn imm7 asm_pattern_load_store_pair
  0b10_101_0_011_0_0000000_00000_00000_00000 or
;

\ stp Xt1, Xt2, [Xn], <imm>
: asm_store_pair_post { Xt1 Xt2 Xn imm7 -- instr }
  Xt1 Xt2 Xn imm7 asm_pattern_load_store_pair
  0b10_101_0_001_0_0000000_00000_00000_00000 or
;

\ stp Xt1, Xt2, [Xn, <imm>]
: asm_store_pair_off { Xt1 Xt2 Xn imm7 -- instr }
  Xt1 Xt2 Xn imm7 asm_pattern_load_store_pair
  0b10_101_0_010_0_0000000_00000_00000_00000 or
;

\ Shared by a lot of load and store instructions.
\ Patches these bits:
\   0b00_000_0_00_00_0_XXXXXXXXX_00_XXXXX_XXXXX
: asm_pattern_load_store { Xt Xn imm9 -- instr_mask }
  Xn                5 lsl { Xn }
  imm9 9 bit_trunc 12 lsl
  Xt or Xn or
;

\ ldr Xt, [Xn, <imm>]!
: asm_load_pre { Xt Xn imm9 -- instr }
  Xt Xn imm9 asm_pattern_load_store
  0b11_111_0_00_01_0_000000000_11_00000_00000 or
;

\ ldr Xt, [Xn], <imm>
: asm_load_post { Xt Xn imm9 -- instr }
  Xt Xn imm9 asm_pattern_load_store
  0b11_111_0_00_01_0_000000000_01_00000_00000 or
;

\ str Xt, [Xn, <imm>]!
: asm_store_pre { Xt Xn imm9 -- instr }
  Xt Xn imm9 asm_pattern_load_store
  0b11_111_0_00_00_0_000000000_11_00000_00000 or
;

\ str Xt, [Xn], <imm>
: asm_store_post { Xt Xn imm9 -- instr }
  Xt Xn imm9 asm_pattern_load_store
  0b11_111_0_00_00_0_000000000_01_00000_00000 or
;

\ ldur Xt, [Xn, <imm>]
: asm_load_off { Xt Xn imm9 -- instr }
  Xt Xn imm9 asm_pattern_load_store
  0b11_111_0_00_01_0_000000000_00_00000_00000 or
;

\ stur Xt, [Xn, <imm>]
: asm_store_off { Xt Xn imm9 -- instr }
  Xt Xn imm9 asm_pattern_load_store
  0b11_111_0_00_00_0_000000000_00_00000_00000 or
;

\ ldur Wt, [Xn, <imm>]
: asm_load_off_32 { Wt Xn imm9 -- instr }
  Wt Xn imm9 asm_pattern_load_store
  0b10_111_0_00_01_0_000000000_00_00000_00000 or
;

\ stur Wt, [Xn, <imm>]
: asm_store_off_32 { Wt Xn imm9 -- instr }
  Wt Xn imm9 asm_pattern_load_store
  0b10_111_0_00_00_0_000000000_00_00000_00000 or
;

\ Shared by register-offset load and store instructions.
\ `scale` must be 0 or 1; if 1, offset is multiplied by 8.
: asm_pattern_load_store_with_register { Xt Xn Xm scale -- instr_mask }
  Xn        5  lsl { Xn }
  Xm        16 lsl { Xm }
  scale <>0 12 lsl \ lsl 3
  Xt or Xn or Xm or
;

\ ldr Xt, [Xn, Xm, lsl <scale>]
: asm_load_with_register { Xt Xn Xm scale -- instr }
  Xt Xn Xm scale asm_pattern_load_store_with_register
  0b11_111_0_00_01_1_00000_011_0_10_00000_00000 or
;

\ str Xt, [Xn, Xm, lsl <scale>]
: asm_store_with_register { Xt Xn Xm scale -- instr }
  Xt Xn Xm scale asm_pattern_load_store_with_register
  0b11_111_0_00_00_1_00000_011_0_10_00000_00000 or
;

\ Shared by some integer arithmetic instructions.
: asm_pattern_arith_reg { Xd Xn Xm -- instr_mask }
  Xn 5  lsl { Xn }
  Xm 16 lsl
  Xd or Xn or
;

\ Shared by some integer arithmetic and load/store instructions.
: asm_pattern_arith_imm { Xd Xn imm12 -- instr_mask }
  Xn                  5 lsl { Xn }
  imm12 12 bit_trunc 10 lsl
  Xd or Xn or
;

: asm_load_byte_off { Wt Wn imm12 -- instr }
  Wt Wn imm12 asm_pattern_arith_imm
  0b00_11_1_0_0_1_01_000000000000_00000_00000 or
;

: asm_store_byte_off { Wt Wn imm12 -- instr }
  Wt Wn imm12 asm_pattern_arith_imm
  0b00_11_1_0_0_1_00_000000000000_00000_00000 or
;

\ neg Xd, Xd
: asm_neg { Xd -- instr }
  Xd 0b1_1_0_01011_00_0_00001_000000_11111_00000 or
;

\ add Xd, Xn, <imm12>
: asm_add_imm { Xd Xn imm12 -- instr }
  Xd Xn imm12 asm_pattern_arith_imm
  0b1_0_0_100010_0_000000000000_00000_00000 or
;

\ add Xd, Xn, Xm
: asm_add_reg { Xd Xn Xm -- instr }
  Xd Xn Xm asm_pattern_arith_reg
  0b1_0_0_01011_00_0_00000_000000_00000_00000 or
;

\ sub Xd, Xn, <imm12>
: asm_sub_imm { Xd Xn imm12 -- instr }
  Xd Xn imm12 asm_pattern_arith_imm
  0b1_1_0_100010_0_000000000000_00000_00000 or
;

\ sub Xd, Xn, Xm
: asm_sub_reg { Xd Xn Xm -- instr }
  Xd Xn Xm asm_pattern_arith_reg
  0b1_1_0_01011_00_0_00000_000000_00000_00000 or
;

\ sub Xd, Xn, Xm, lsl 3
: asm_sub_reg_words { Xd Xn Xm -- instr }
  Xd Xn Xm asm_sub_reg
  0b0_0_0_00000_00_0_00000_000011_00000_00000 or
;

\ mul Xd, Xn, Xm
: asm_mul { Xd Xn Xm -- instr }
  Xd Xn Xm asm_pattern_arith_reg
  0b1_00_11011_000_00000_0_11111_00000_00000 or
;

\ sdiv Xd, Xn, Xm
: asm_sdiv { Xd Xn Xm -- instr }
  Xd Xn Xm asm_pattern_arith_reg
  0b1_0_0_11010110_00000_00001_1_00000_00000 or
;

\ msub Xd, Xn, Xm, Xa
: asm_msub { Xd Xn Xm Xa -- instr }
  Xn  5 lsl { Xn }
  Xm 16 lsl { Xm }
  Xa 10 lsl
  Xd or Xn or Xm or
  0b1_00_11011_000_00000_1_00000_00000_00000 or
;

: asm_pattern_arith_csel { Xd Xn Xm cond -- instr_mask }
  cond 12 lsl { cond }
  Xd Xm Xn asm_pattern_arith_reg
  cond or
;

\ csel Xd, Xn, Xm, <cond>
: asm_csel { Xd Xn Xm cond -- instr }
  Xd Xm Xn cond asm_pattern_arith_csel
  0b1_0_0_11010100_00000_0000_0_0_00000_00000 or
;

\ csneg Xd, Xn, Xm, <cond>
: asm_csneg { Xd Xn Xm cond -- instr }
  Xd Xm Xn cond asm_pattern_arith_csel
  0b1_1_0_11010100_00000_0000_0_1_00000_00000 or
;

\ asr Xd, Xn, imm6
: asm_asr_imm { Xd Xn imm6 -- instr }
  Xd Xn imm6 asm_pattern_arith_reg
  0b1_00_100110_1_000000_111111_00000_00000 or
;

\ asr Xd, Xn, Xm
: asm_asr_reg { Xd Xn Xm -- instr }
  Xd Xn Xm asm_pattern_arith_reg
  0b1_0_0_11010110_00000_0010_10_00000_00000 or
;

\ Arithmetic (sign-preserving) right shift.
: asr { i0 bits -- i1 } [
  0 0 1 asm_asr_reg comp_instr \ asr x0, x0, x1
  1                 comp_args_set
] ;

\ eor Xd, Xn, Xm
: asm_eor_reg { Xd Xn imm12 -- instr }
  Xd Xn imm12 asm_pattern_arith_imm
  0b1_10_01010_00_0_00000_000000_00000_00000 or
;

\ cmp Xn, Xm
: asm_cmp_reg { Xn Xm -- instr }
  Xn  5 lsl { Xn }
  Xm 16 lsl
  Xn or
  0b1_1_1_01011_00_0_00000_000_000_00000_11111 or
;

\ b <off>
: asm_branch { off26 -- instr }
  off26
  2  lsr       \ Offset is implicitly times 4.
  26 bit_trunc \ Offset may be negative.
  0b0_00_101_00000000000000000000000000 or
;

\ b.<cond> <off>
: asm_branch_cond { off19 cond -- instr }
  off19 2 asr 19 bit_trunc 5 lsl \ Implicitly times 4.
  cond or
  0b010_101_00_0000000000000000000_0_0000 or
;

\ `off19` offset is implicitly times 4 and can be negative.
: asm_pattern_cmp_branch { Xt off19 -- instr_mask }
  off19 2 lsr 19 bit_trunc 5 lsl
  Xt or
;

\ cbz x1, <off>
: asm_cmp_branch_zero { Xt off19 -- instr }
  Xt off19 asm_pattern_cmp_branch
  0b1_011010_0_0000000000000000000_00000 or
;

\ cbnz x1, <off>
: asm_cmp_branch_not_zero { Xt off19 -- instr }
  Xt off19 asm_pattern_cmp_branch
  0b1_011010_1_0000000000000000000_00000 or
;

\ `off9` offset is implicitly times 4 and can be negative.
: asm_pattern_cmp_branch_cond { Xt off9 cond -- instr_mask }
  off9 2 lsr 9 bit_trunc 5  lsl { off9 }
  cond                   21 lsl
  Xt or off9 or
;

\ Not supported on M3 Pro.
\ cb.<cond> Xt, <imm>, <off>
: asm_cmp_imm_branch { Xt imm6 off9 cond -- instr }
  Xt off9 cond asm_pattern_cmp_branch_cond { mask }
  imm6 15 lsl
  mask or
  0b1_1110101_000_000000_0_000000000_00000 or
;

\ Not supported on M3 Pro.
\ cb.<cond> Xt, Xm, <off>
: asm_cmp_reg_branch { Xt Xm off9 cond -- instr }
  Xt off9 cond asm_pattern_cmp_branch_cond { mask }
  Xm 16 lsl
  mask or
  0b1_1110100_000_00000_00_000000000_00000 or
;

\ eor Xd, Xd, Xd
: asm_zero_reg { Xd -- instr } Xd Xd Xd asm_eor_reg ;

: asm_mov_reg { Xd Xm -- instr }
  Xm 16 lsl
  Xd or
  0b1_01_01010_00_0_00000_000000_11111_00000 or
;

\ Immediate must be unsigned.
: asm_mov_imm { Xd imm -- instr }
  imm 5 lsl
  Xd or
  0b1_10_100101_00_0000000000000000_00000 or
;

\ mvn Xd, Xm
: asm_mvn { Xd Xm -- instr }
  Xm 16 lsl
  Xd or
  0b1_01_01010_00_1_00000_000000_11111_00000 or
;

\ `ret x30`; requires caution with SP / FP.
0b110_101_1_0_0_10_11111_0000_0_0_11110_00000 let: asm_ret

0b110_101_01000000110010_0000_000_11111 let: asm_nop

\ svc 666
0b110_101_00_000_0000001010011010_000_01 let: asm_svc

\ ## Arithmetic

: negate { i0 -- i1 } [
  0 asm_neg comp_instr \ neg x0, x0
  1         comp_args_set
] ;

: + { i0 i1 -- i2 } [
  0 0 1 asm_add_reg comp_instr \ add x0, x0, x1
  1                 comp_args_set
] ;

: - { i0 i1 -- i2 } [
  0 0 1 asm_sub_reg comp_instr \ sub x0, x0, x1
  1                 comp_args_set
] ;

: * { i0 i1 -- i2 } [
  0 0 1 asm_mul comp_instr \ mul x0, x0, x1
  1             comp_args_set
] ;

: / { i0 i1 -- i2 } [
  0 0 1 asm_sdiv comp_instr \ sdiv x0, x0, x1
  1              comp_args_set
] ;

: mod { i0 i1 -- i2 } [
  2                comp_clobber
  2 0 1   asm_sdiv comp_instr \ sdiv x2, x0, x1
  0 2 1 0 asm_msub comp_instr \ msub x0, x2, x1, x0
  1                comp_args_set
] ;

: /mod { dividend divisor -- rem quo } [
  2                   comp_clobber
  2 1     asm_mov_reg comp_instr \ mov x2, x1
  1 0 2   asm_sdiv    comp_instr \ sdiv x1, x0, x2
  0 1 2 0 asm_msub    comp_instr \ msub x0, x1, x2, x0
  2                   comp_args_set
] ;

: abs { int -- +int } [
  0            asm_cmp_zero comp_instr \ cmp x0, 0
  0 0 0 ASM_PL asm_csneg    comp_instr \ cneg x0, x0, pl
  1                         comp_args_set
] ;

: min { i0 i1 -- i0|i1 } [
  0 1          asm_cmp_reg comp_instr \ cmp x0, x1
  0 0 1 ASM_LT asm_csel    comp_instr \ csel x0, x0, x1, lt
  1                        comp_args_set
] ;

: max { i0 i1 -- i0|i1 } [
  0 1          asm_cmp_reg comp_instr \ cmp x0, x1
  0 0 1 ASM_GT asm_csel    comp_instr \ csel x0, x0, x1, gt
  1                        comp_args_set
] ;

: align_down { len wid -- len } wid negate len and ;
: align_up   { len wid -- len } wid dec len + wid align_down ;

8 let: cell
: cells { len -- size } len 3 lsl ;
: /cells { size -- len } size 3 asr ;
: +cell { adr -- adr } adr cell + ;
: -cell { adr -- adr } adr cell - ;

\ ## Assembler continued

\ lsl Xd, Xn, <imm>
: asm_lsl_imm { Xd Xn imm6 -- instr }
  imm6 negate 64 mod 6 bit_trunc 16 lsl { immr }
  63 imm6 -          6 bit_trunc 10 lsl { imms }
  Xn 5 lsl
  Xd or imms or immr or
  0b1_10_100110_1_000000_000000_00000_00000 or
;

\ tbz Xt, <bit>, <imm>
: asm_test_bit_branch_zero { Xt bit imm14 -- instr }
  imm14 2 lsr 14 bit_trunc 5 lsl { imm14 } \ Implicitly times 4.
  bit   5 lsr             31 lsl { b5 }
  bit         5 bit_trunc 19 lsl \ b40
  Xt or imm14 or b5 or
  0b0_01_101_1_0_00000_00000000000000_00000 or
;

\ orr Xd, Xn, #(1 << bit)
\
\ TODO add general-purpose `orr`.
: asm_orr_bit { Xd Xn bit -- instr }
  64 bit - 16 lsl { immr }
  Xn        5 lsl
  Xd or immr or
  0b1_01_100100_1_000000_000000_00000_00000 or
;

: asm_comp_cset_reg { cond } ( E: i0 i1 -- bool )
  0 1    asm_cmp_reg comp_instr \ cmp x0, x1
  0 cond asm_cset    comp_instr \ cset x0, <cond>
  1                  comp_args_set
;

: asm_comp_cset_zero { cond } ( E: num -- bool )
  0      asm_cmp_zero comp_instr \ cmp x0, 0
  0 cond asm_cset     comp_instr \ cset x0, <cond>
  1                   comp_args_set
;

\ ## Numeric comparison

\ https://gforth.org/manual/Numeric-comparison.html
: =   { i0 i1 -- bool } [ ASM_EQ asm_comp_cset_reg  ] ;
: <>  { i0 i1 -- bool } [ ASM_NE asm_comp_cset_reg  ] ;
: >   { i0 i1 -- bool } [ ASM_GT asm_comp_cset_reg  ] ;
: <   { i0 i1 -- bool } [ ASM_LT asm_comp_cset_reg  ] ;
: <=  { i0 i1 -- bool } [ ASM_LE asm_comp_cset_reg  ] ;
: >=  { i0 i1 -- bool } [ ASM_GE asm_comp_cset_reg  ] ;
: <0  { num   -- bool } [ ASM_LT asm_comp_cset_zero ] ; \ Or `MI`.
: >0  { num   -- bool } [ ASM_GT asm_comp_cset_zero ] ;
: <=0 { num   -- bool } [ ASM_LE asm_comp_cset_zero ] ;
: >=0 { num   -- bool } [ ASM_GE asm_comp_cset_zero ] ; \ Or `PL`.

: within { num one two -- bool }
  one num > { one }
  num two >
  one or =0
;

: odd { i0 -- bool } i0 1 and ;

\ ## Memory load / store

: @ { adr -- val } [
  0 0 0 asm_load_off comp_instr \ ldur x0, [x0]
  1                  comp_args_set
] ;

: ! { val adr } [
  0 1 0 asm_store_off comp_instr \ stur x0, [x1]
] ;

: @2 { adr -- val0 val1 } [
  0 1 0 0 asm_load_pair_off comp_instr \ ldp x0, x1, [x0]
  2                         comp_args_set
] ;

: !2 { val0 val1 adr -- } [
  0 1 2 0 asm_store_pair_off comp_instr \ stp x0, x1, [x2]
] ;

\ 32-bit version of `!`. Used for patching instructions.
: !32 { val adr } [
  0 1 0 asm_store_off_32 comp_instr \ str w0, x1
] ;

: off! { adr } false adr ! ;
: on!  { adr } true adr ! ;

\ ## Stack introspection and manipulation
\
\ Under the register calling convention, the Forth data stack
\ is used only in top-level interpretation. Because of this,
\ we don't reserve any registers for the stack floor / top.
\
\ Instead, the stack is accessed directly on the interpreter
\ object, which IS unavoidably stored in a special register.
\ Compared to the stack CC, the reg-based CC gets away with
\ fewer special registers.
\
\ Note: our stack is empty-ascending.
\
\ SYNC[interp_stack_offset].
\ SYNC[stack_field_offsets].

16 let: INTERP_INTS_FLOOR
24 let: INTERP_INTS_TOP

\ add <reg>, x27, INTERP_INTS_TOP
:: stack_top_ptr ( E: -- adr )
  comp_next_arg_reg
  ASM_REG_INTERP INTERP_INTS_TOP asm_add_imm comp_instr
;

: stack_top_ptr { -- adr } stack_top_ptr ;

\ ldur <reg>, [x27, INTERP_INTS_FLOOR]
:: stack_floor ( E: -- adr )
  comp_next_arg_reg
  ASM_REG_INTERP INTERP_INTS_FLOOR asm_load_off comp_instr
;

: stack_floor { -- adr } stack_floor ;

\ ldur <reg>, [x27, INTERP_INTS_TOP]
:: stack_top ( E: -- adr )
  comp_next_arg_reg
  ASM_REG_INTERP INTERP_INTS_TOP asm_load_off comp_instr
;

: stack_top { -- adr } stack_top ;

: stack@ { -- val } stack_top @ ;

\ Updates the stack top address in the interpreter.
: stack! { adr } [
  \ stur x0, [x27, INTERP_INTS_TOP]
  0 ASM_REG_INTERP INTERP_INTS_TOP asm_store_off comp_instr
] ;

: stack_at      { ind  -- adr } ind cells stack_floor + ;
: stack_at@     { ind  -- val } ind stack_at @ ;
: stack_len     {      -- len } stack_top stack_floor - /cells ;
: stack_clear   {      --     } stack_floor stack! ;
: stack_set_len { len  --     } len stack_at stack! ;
: stack+        { diff --     } diff dec cells          stack_top      + stack! ;
: stack-        { diff --     } diff dec cells { diff } stack_top diff - stack! ;

\ Used by control structures such as conditionals.
\
\ Under the reg-based CC, compiled code does NOT use the Forth data stack.
\ However, conditionals and loops need to use it for control information
\ during compilation.
\
\ Each control structure pushes one or several "frames". Each frame consists
\ of several inputs followed by a procedure which takes those inputs. Frame
\ size is unknown to outside code; the knowledge of how many inputs to "pop"
\ is hidden inside each frame's procedure. In other words, arities vary.
\
\ We can't really communicate any of this to the compiler,
\ so we have to fall back on regular old push and pop here.

\ ldur <top>, [x27, INTERP_INTS_TOP]
: asm_stack_load { adr -- instr }
  adr ASM_REG_INTERP INTERP_INTS_TOP asm_load_off
;

\ stur <top>, [x27, INTERP_INTS_TOP]
: asm_stack_store { adr -- instr }
  adr ASM_REG_INTERP INTERP_INTS_TOP asm_store_off
;

\ Usage:
\
\   stack>        unary
\   stack> stack> binary
\
\ Mind the backwards order:
\
\   10 20 30 >>stack
\   stack> stack> stack> { thirty twenty ten }
\
\ Also see `stack{` which is more convenient and less error-prone.
:: stack> ( E: -- val )
  comp_next_arg_reg { arg }
  ASM_REG_SCRATCH   { top }

  top                        comp_clobber
  top        asm_stack_load  comp_instr
  arg top -8 asm_load_pre    comp_instr \ ldr <arg>, [<top>, -8]!
  top        asm_stack_store comp_instr
;

: stack> { -- val } stack> ;

\ Also see `:: >>stack`, which is defined _way_ later in the file,
\ when we have access to loops. Unlike this version it's variadic.
: >stack { val } [
  1                     comp_clobber
  1     asm_stack_load  comp_instr \ ldur x1, [x27, INTERP_INTS_TOP]
  0 1 8 asm_store_post  comp_instr \ str x0, [x1], 8
  1     asm_stack_store comp_instr \ stur x1, [x27, INTERP_INTS_TOP]
] ;

\ ## Characters and strings

: c@ { str -- char } [
  0 0 0 asm_load_byte_off comp_instr \ ldrb x0, [x0]
  1                       comp_args_set
] ;

: c! { char str -- } [
  0 1 0 asm_store_byte_off comp_instr \ strb x0, [x1]
] ;

: parse_char { -- char } parse_word { buf -- } buf c@ ;

: char' { -- char } ( E: "str" -- char ) parse_char ;

:: char' ( C: "str" -- ) ( E: -- char )
  parse_char        { char }
  comp_next_arg_reg { reg }
  char reg comp_load
;

\ Interpretation semantics of standard `s"`.
\ The parser ensures null-termination.
: parse_str { -- cstr len } char' " parse ;
: parse_cstr { -- cstr } parse_str { cstr -- } cstr ;

\ Compilation semantics of standard `s"`.
: comp_str ( C: "str" -- ) ( E: -- cstr len )
  parse_str          { buf len }
  len inc            { cap } \ Reserve terminating null byte.
  buf cap alloc_data { adr }
  comp_next_arg_reg  { RS }
  comp_next_arg_reg  { RL }
  adr RS comp_page_addr \ `adrp RS, <page>` & `add RS, RS, <pageoff>`
  len RL comp_load      \ ldr RL, <len>
;

: comp_cstr ( C: "str" -- ) ( E: -- cstr )
  parse_str          { buf len }
  len inc            { cap } \ Reserve terminating null byte.
  buf cap alloc_data { adr }
  comp_next_arg_reg  { reg }
  adr reg comp_page_addr \ `adrp <reg>, <page>` & `add <reg>, <reg>, <pageoff>`
;

:  " { -- cstr len } ( E: "str" -- cstr len ) parse_str ;
:: " ( C: "str" -- ) ( E: -- cstr len )       comp_str ;

:  c" { -- cstr }     ( E: "str" -- cstr ) parse_cstr ;
:: c" ( C: "str" -- ) ( E: -- cstr )       comp_cstr ;

\ ## Memory
\
\ The program is implicitly linked with `libc`,
\ so we get to use C stdlib procedures.

2 1 extern: calloc  ( len size -- addr )
1 1 extern: malloc  ( size -- addr )
1 0 extern: free    ( addr -- )
3 0 extern: memset  ( buf char len -- )
3 0 extern: memcpy  ( dst src len -- )
3 0 extern: memmove ( dst src len -- )

1 1 extern: strdup  ( cstr -- cstr )
1 1 extern: strlen  ( cstr -- len )
3 1 extern: strncmp ( str0 str1 len -- )
3 0 extern: strncpy ( dst src str_len -- )
3 0 extern: strlcpy ( dst src buf_len -- )

: move  { src dst len  } dst src len strncpy ;
: fill  { buf len char } buf char len memset ;
: erase { buf len      } buf len 0 fill ;
: blank { buf len      } buf len 32 fill ;

: compare { str0 len0 str1 len1 -- direction }
  len0 len1 min { len }
  str0 str1 len strncmp
;

: str=  { str0 len0 str1 len1 -- bool } str0 len0 str1 len1 compare =0 ;
: str<  { str0 len0 str1 len1 -- bool } str0 len0 str1 len1 compare <0 ;
: str<> { str0 len0 str1 len1 -- bool } str0 len0 str1 len1 compare <>0 ;

\ ## Variables, buffers, extern vars

\ Asks for a register and compiles this idiom:
\
\   adrp <reg>, <page>
\   add <reg>, <reg>, <pageoff>
\
\ Used for taking an address of a data entry
\ in the code heap allocated via `alloc_data`.
: comp_extern_addr { adr } ( E: -- adr )
  comp_next_arg_reg { reg }
  adr reg comp_page_addr
;

\ Used for loading a value in a data entry
\ or GOT entry in the code heap.
: comp_extern_load { adr } ( E: -- val )
  comp_next_arg_reg { reg }
  adr reg comp_page_load \ adrp & ldr
;

\ Used for loading the value behind an address
\ stored in a GOT entry in the code heap.
: comp_extern_load_load { adr } ( E: -- val )
  comp_next_arg_reg { reg }
  adr reg                comp_page_load \ adrp & ldr
  reg reg 0 asm_load_off comp_instr     \ ldur x0, [x0]
;

: init_data_word { name name_len adr }
  name name_len colon
    0 1 comp_signature ( E: -- ptr )
    adr comp_extern_addr
  semicolon

  name name_len colon_colon
    adr comp_push compile' comp_extern_addr
  semicolon
;

\ Similar to standard `variable`, but takes an initial value like `let:`.
: var: { val } ( C: "name" -- ) ( E: -- ptr )
  parse_word { str len }
  nil cell alloc_data { adr }
  val adr !
  str len adr init_data_word
  [ not_comp_only ]
;

: comp_buf { adr cap } ( E: -- adr cap )
  comp_next_arg_reg { RA }
  comp_next_arg_reg { RL }
  adr RA comp_page_addr
  cap RL comp_load
;

\ Similar to the standard idiom `create <name> N allot`.
\ Creates a global variable which refers to a buffer of
\ at least the given size in bytes.
\
\ TODO dedup code with `init_data_word`.
: buf: { cap } ( C: "name" -- ) ( E: -- adr cap )
  parse_word { str len }
  nil cap alloc_data { adr }

  str len colon
    0 2 comp_signature ( E: -- ptr )
    adr cap comp_buf
  semicolon

  str len colon_colon
    adr comp_push cap comp_push compile' comp_buf
  semicolon

  [ not_comp_only ]
;

\ Shortcut for the standard idiom `create <name> N allot`.
\ Like `buf:` but doesn't return capacity.
: mem: { cap } ( C: "name" -- ) ( E: -- adr )
  nil cap alloc_data { adr }
  parse_word adr init_data_word
  [ not_comp_only ]
;

\ Shortcut for the standard idiom `create <name> N cells allot`.
: cells: { len } ( C: "name" -- ) ( E: -- adr ) len cells execute' mem: ;

4096 buf: BUF

\ Analogous to `extern:`, but for external variables rather than procedures.
: extern_val: ( C: "ours" "extern" -- ) ( E: -- val )
  parse_word { str len }

  \ Get a stable name location because we're about
  \ to parse one more word, which overwrites `str`.
  BUF { buf cap } buf str cap strlcpy

  parse_word extern_got { adr }

  buf len colon
    0 1 comp_signature ( E: -- val )
    adr comp_extern_load_load
  semicolon

  buf len colon_colon
    adr comp_push compile' comp_extern_load_load
  semicolon

  [ not_comp_only ]
;

\ ## IO
\
\ Having access to `libc` spares us from having
\ to implement these procedures or provide them
\ as intrinsics.

extern_val: stdin  __stdinp
extern_val: stdout __stdoutp
extern_val: stderr __stderrp

4 0 extern: fwrite   ( src size len file -- )
1 0 extern: putchar  ( char -- )
2 0 extern: fputc    ( char file -- )
2 0 extern: fputs    ( cstr file -- )
1 0 extern: printf   ( … fmt -- )
2 0 extern: fprintf  ( … file fmt -- )
3 0 extern: snprintf ( … buf cap fmt -- )
1 0 extern: fflush   ( file -- )

: emit     { char }    char putchar ;
: eputchar { char }    char stderr fputc ;
: puts     { cstr }    cstr stdout fputs ;
: eputs    { cstr }    cstr stderr fputs ;
: flush                stdout fflush ;
: eflush               stderr fflush ;
: type     { str len } str 1 len stdout fwrite ;
: etype    { str len } str 1 len stderr fwrite ;
: lf                   10 putchar ; \ Renamed from `cr`.
: elf                  10 eputchar ;
: space                32 putchar ;
: espace               32 eputchar ;

:  log" parse_cstr puts ;
:: log" comp_cstr compile' puts ;

:  elog" parse_cstr eputs ;
:: elog" comp_cstr compile' eputs ;

\ ## Exceptions — basic
\
\ Exception definitions are split. See additional words below
\ which support message formatting via the C "printf" family.
\
\ We dedicate one special register to an error value, which
\ is either zero or an address of a null-delimited string.
\ To "throw", we have to store a string into that register
\ and instruct the compiler to insert a "try" check.

\ The annotation `throws` makes the compiler insert an error check
\ after any call to this procedure, and makes it "contagious": all
\ callers automatically receive "throws".
: throw { cstr } [
  throws
  ASM_REG_ERR 0 asm_mov_reg comp_instr \ mov x28, x0
] ;

\ Usage:
\
\   throw" some_error_msg"
\
\ Also see `sthrowf"` for error message formatting.
:  throw" ( E: "str" -- ) parse_cstr throw ;
:: throw" ( C: "str" -- ) comp_cstr compile' throw ;

\ ## Conditionals
\
\ General idea:
\
\   #if        \ cbz <outside>
\     if_true
\   #end
\   outside
\
\   #if        \ cbz <if_false>
\     if_true
\   #else      \ b <outside>
\     if_false
\   #end
\   outside
\
\ Similarly to Gforth, our conditionals use the Forth data stack
\ to store control information at compile time. When terminated,
\ they use this information to "fixup" reserved instructions.
\
\ Unlike other Forth systems, we also support chains
\ of "elif" terminated with a single "end".

: pop_execute stack> execute ;
: reserve_instr ASM_PLACEHOLDER comp_instr ;
: reserve_here {     -- adr } here { out } ASM_PLACEHOLDER comp_instr out ;
: off_to_pc    { adr -- off } here             adr - ; \ For forward jumps.
: off_from_pc  { adr -- off } here { out } adr out - ; \ For backward jumps.
: patch_branch_forward { adr } adr off_to_pc   asm_branch adr !32 ; \ b <off>
: patch_branch_back    { adr } adr off_from_pc asm_branch adr !32 ; \ b <off>

0 var: COND_HAS

\ cbz x0, <else|end>
: if_patch ( C: adr -- )
  stack>        { adr }
  adr off_to_pc { off }
  0 off asm_cmp_branch_zero adr !32
;

\ cbnz x0, <else|end>
: ifn_patch ( C: adr -- )
  stack>        { adr }
  adr off_to_pc { off }
  0 off asm_cmp_branch_not_zero adr !32
;

: if_done ( C: cond_prev        -- ) stack> COND_HAS ! ;
: if_pop  ( C: fun_prev adr_if  -- ) if_patch  stack> execute ;
: ifn_pop ( C: fun_prev adr_ifn -- ) ifn_patch stack> execute ;

: elif_init ( C: -- fun_exec adr_if ) ( E: pred -- )
  comp_barrier \ Clobber / relocate locals.
  reserve_here { adr }
  ' pop_execute >stack
  adr           >stack
;

:: #elif ( C: -- fun_exec adr_if fun_if ) ( E: pred -- )
  c" when calling `#elif`" 1 comp_args_valid 0 comp_args_set
  elif_init ' if_pop >stack
;

:: #elifn ( C: -- fun_exec adr_if fun_if ) ( E: pred -- )
  c" when calling `#elifn`" 1 comp_args_valid 0 comp_args_set
  elif_init ' ifn_pop >stack
;

: if_init ( C: -- cond fun )
  COND_HAS @ { cond } COND_HAS on!
  cond      >stack
  ' if_done >stack
;

\ With this strange setup, `#end` pops the top control frame,
\ which pops the next control frame, and so on. This allows us
\ to pop any amount of control constructs with a single `#end`.
\ Prepending `if_done` terminates this chain.
:: #if ( C: -- cond fun_done fun_exec adr_if fun_if ) ( E: pred -- )
  c" when calling `#if`" 1 comp_args_valid 0 comp_args_set
  if_init elif_init ' if_pop >stack
;

:: #ifn ( C: -- cond fun_done fun_exec adr_ifn fun_ifn ) ( E: pred -- )
  c" when calling `#ifn`" 1 comp_args_valid 0 comp_args_set
  if_init elif_init ' ifn_pop >stack
;

: else_pop ( C: fun_prev adr_else -- )
  stack> patch_branch_forward stack> execute
;

:: #else ( C: fun_exec adr_if fun_if -- adr_else fun_else )
  stack> stack> stack> { fun_if adr_if fun_exec }

  c" when calling `#else`" 0 comp_args_valid
  comp_barrier \ Clobber / relocate locals.
  reserve_here { adr_else }

  ' nop      >stack \ Replace `fun_exec` to prevent further chaining.
  adr_if     >stack
  fun_if     execute
  adr_else   >stack
  ' else_pop >stack
;

\ TODO: use raw instruction addresses and `blr`
\ instead of execution tokens and `execute`.
:: #end ( C: fun -- ) comp_barrier pop_execute ;
:  #end { fun }       fun          execute ;

\ ## Loops
\
\ Unlike other Forth systems, we implement termination of arbitrary
\ control structures with a generic `#end`. This works for all loops
\ and eliminates the need to remember many different terminators.
\
\ Our solution isn't complicated, either. We simply push procedures
\ with their arguments to the control stack, and pop the latest one
\ with `#end`. Each procedure pops its arguments and does something,
\ then pops the next procedure in the chain. The cascade terminates
\ when encountering a procedure which doesn't pop a preceding one.

\ Stack position of a special location in the top loop frame
\ on the control stack. This is where auxiliary structures
\ such as `#leave` place their own control frames.
0 var: LOOP_FRAME

: loop_frame! ( C: frame_ind -- ) stack> LOOP_FRAME ! ;

: loop_frame_init ( C: -- frame_ind frame_fun )
  LOOP_FRAME @  >stack
  ' loop_frame! >stack
  stack_len LOOP_FRAME !
;

\ Used by auxiliary loop constructs such as "leave" and "while"
\ to insert their control frame before a loop control frame.
\ This allows to pop the control frames in the right order
\ with a single "end" word.
: loop_aux
  ( C: prev… <loop> …rest… <frame> stack_len -- prev… <frame> <loop> …rest )
  stack>    { stack_len_0 }
  stack_len { stack_len_1 }

  LOOP_FRAME @
  #ifn
    throw" auxiliary loop constructs require an ancestor loop frame"
  #end

  stack_len_1  stack_len_0 - { frame_len }
  frame_len    cells         { frame_size }
  stack_len_0  stack_at      { frame_sp_prev }
  stack_len_1  stack_at      { frame_sp_temp }
  LOOP_FRAME @ stack_at      { loop_sp_prev }
  loop_sp_prev frame_size  + { loop_sp_next }

  \ Need a backup copy of the frame we're moving,
  \ because we're about to overwrite that data.
  frame_len stack+
  frame_sp_temp frame_sp_prev frame_size memmove

  \ Move everything starting with the loop frame,
  \ making space for the aux frame we're inserting.
  frame_sp_temp loop_sp_next - { rest_size }
  loop_sp_next loop_sp_prev rest_size memmove

  \ The aux frame is now behind the moved loop frame.
  loop_sp_prev frame_sp_temp frame_size memmove
  frame_len stack-

  \ Subsequent aux constructs will be inserted after the new frame.
  LOOP_FRAME @ frame_len + LOOP_FRAME !
;

: loop_end ( C: adr -- )
  stack> off_from_pc asm_branch comp_instr \ b <beg>
  comp_barrier                             \ Clobber / relocate locals.
;

: loop_pop ( C: fun_prev …aux… adr_beg -- ) loop_end pop_execute ;

\ Implementation note. Each loop frame is "split" in two sub-frames:
\ the "prev frame" and the "current frame". Auxiliary loop constructs
\ such as "leave" and "while" insert their own frames in-between these
\ subframes. See `loop_aux`.
:: #loop ( C: -- ind_frame fun_frame adr_loop fun_loop )
  comp_barrier \ Clobber / relocate locals.
  here { adr }
  loop_frame_init adr >stack
  ' loop_pop          >stack
;

\ Can be used in any loop.
:: #leave ( C: prev… <loop> …rest -- prev… adr[leave] fun[leave] <loop> …rest )
  comp_barrier \ Clobber / relocate locals.
  stack_len    { len }
  reserve_here >stack
  ' else_pop   >stack
  len          >stack
  loop_aux
;

\ Can be used in any loop.
:: #while ( C: prev… <loop> …rest -- prev… adr[while] fun[while] <loop> …rest )
  c" when calling `#while`" 1 comp_args_valid 0 comp_args_set
  comp_barrier \ Clobber / relocate locals.
  stack_len    { len }
  reserve_here >stack
  ' if_pop     >stack
  len          >stack
  loop_aux
;

\ Assumes that the top control frame is from `#loop`.
:: #until ( C: fun -- )
  c" when calling `#until`" 1 comp_args_valid 0 comp_args_set
  comp_barrier \ Clobber / relocate locals.
  0 8 asm_cmp_branch_zero comp_instr \ cbnz x0, 8
  stack> execute
;

: count_loop_pop { cond } ( fun_prev …aux… adr_cond adr_beg -- )
  loop_end    \ b <beg>; clobber / relocate locals.
  stack> { adr }
  adr off_to_pc cond asm_branch_cond adr !32 \ b.<cond> <end>
  pop_execute \ Pop auxiliaries; restore the previous loop frame.
;

: for_countdown_loop_pop ( fun_prev …aux… adr_cond adr_beg cur_loc -- )
  stack> { cur }
  ASM_LE count_loop_pop  \ b <beg>; retropatch b.<cond> <end>
  cur    comp_local_free \ Don't need a register for this local anymore.
;

: for_countdown_loop_init
  { cur }
  ( C: cur_loc -- ind_frame fun_frame adr_cond adr_beg fun_for cur_loc )
  ( E: ceil -- )
  loop_frame_init ( -- ind_frame fun_frame )

  0 cur             comp_local_set \ mov <cur>, x0 | str x0, [FP, <cur>]
  comp_barrier                     \ Clobber / relocate locals.
  here { adr_beg }                 \ Skip above instr when repeating.
  0 cur             comp_local_get \ mov x0, <cur> | ldr x0, [FP, <cur>]
  0 asm_cmp_zero    comp_instr     \ cmp x0, 0
  here { adr_cond } reserve_instr  \ b.<cond> <end>
  0 0 1 asm_sub_imm comp_instr     \ sub x0, x0, 1
  0 cur             comp_local_set \ mov <cur>, x0 | str x0, [FP, <cur>]

  adr_cond                 >stack
  adr_beg                  >stack
  cur                      >stack
  ' for_countdown_loop_pop >stack
;

\ A count-down-by-one loop. Analogue of the non-standard
\ `for … next` loop. Usage; ceiling must be positive:
\
\   123
\   #for
\     log" looping"
\   #end
\
\ Anton Ertl circa 1994:
\
\ > This is the preferred loop of native code compiler writers
\   who are too lazy to optimize `?do` loops properly.
:: #for
  ( C: -- ind_frame fun_frame … adr_cond adr_beg fun_for cur_loc )
  ( E: ceil -- )
  c" when calling `#for`" 1 comp_args_valid 0 comp_args_set
  comp_anon_local for_countdown_loop_init
;

\ Like `#for` but requires a local name to make the index
\ accessible. Usage; ceiling must be positive:
\
\   123
\   -for: ind
\     ind .
\   #end
:: -for:
  ( C: "name" -- frame fun_frame … loc adr_cond adr_beg fun_pop )
  ( E: ceil -- )
  c" when calling `+for:`" 1 comp_args_valid 0 comp_args_set
  parse_word comp_named_local for_countdown_loop_init
;

: for_countup_loop_pop ( fun_prev …aux… adr_cond adr_beg loc_lim loc_cur -- )
  stack> stack> { cur lim }

  0 lim             comp_local_get \ mov x0, <lim> | ldr x0, [FP, <lim>]
  1 cur             comp_local_get \ mov x1, <cur> | ldr x1, [FP, <cur>]
  1 1 1 asm_add_imm comp_instr     \ add x1, x1, 1
  ASM_LE count_loop_pop            \ b <beg>; retropatch b.<cond> <end>

  lim comp_local_free \ Don't need a register for this local anymore.
;

:: +for:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg loc_lim loc_cur fun_pop )
  ( E: ceil floor -- )
  c" when calling `+for:`" 2 comp_args_valid 0 comp_args_set

  loop_frame_init             ( -- ind_frame fun_frame )
  comp_anon_local             { lim } \ ceil  = limit
  parse_word comp_named_local { cur } \ floor = cursor

  0 lim             comp_local_set \ mov <lim>, x0 | str x0, [FP, <lim>]
  comp_barrier                     \ Clobber / relocate locals.
  here { adr_beg }                 \ Skip above instr when repeating.
  1 cur             comp_local_set \ mov <cur>, x1 | str x1, [FP, <cur>]
  comp_barrier                     \ Tell lazy-ass compiler to commit local set.
  0 1 asm_cmp_reg   comp_instr     \ cmp x0, x1
  here { adr_cond } reserve_instr  \ b.<cond> <end>

  adr_cond               >stack
  adr_beg                >stack
  lim                    >stack
  cur                    >stack
  ' for_countup_loop_pop >stack
;

: loop_countup_pop
  ( fun_prev …aux… adr_cond adr_beg loc_lim loc_cur loc_step )
  stack> stack> stack> { step cur lim }

  0 lim             comp_local_get \ mov x0, <lim>  | ldr x0, [FP, <lim>]
  1 cur             comp_local_get \ mov x1, <cur>  | ldr x1, [FP, <cur>]
  2 step            comp_local_get \ mov x2, <step> | ldr x2, [FP, <step>]
  1 1 2 asm_add_reg comp_instr     \ add x1, x1, x2
  ASM_LE count_loop_pop            \ b <beg>; retropatch b.<cond> <end>

  \ Don't need registers for these locals anymore.
  lim  comp_local_free
  step comp_local_free
;

\ Similar to the standard `?do ... +loop`, but terminated with `#end`
\ like other loops. Takes a name to make the index accessible. Usage:
\
\   ceil floor step +loop: ind ind . #end
\   123  23    3    +loop: ind ind . #end
:: +loop:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg loc_cur loc_step fun_pop )
  ( E: ceil floor step -- )
  c" when calling `+loop:`" 3 comp_args_valid 0 comp_args_set
  loop_frame_init

  comp_anon_local             { lim } \ ceil  = limit
  parse_word comp_named_local { cur } \ floor = cursor
  comp_anon_local             { step }

  0 lim             comp_local_set \ mov <lim>,  x0 | str x0, [FP, <lim>]
  2 step            comp_local_set \ mov <step>, x2 | str x2, [FP, <step>]
  comp_barrier                     \ Clobber / relocate locals.
  here { adr_beg }                 \ Skip above instrs when repeating.
  1 cur             comp_local_set \ mov <cur>,  x1 | str x1, [FP, <cur>]
  comp_barrier                     \ Tell lazy-ass compiler to commit local set.
  0 1 asm_cmp_reg   comp_instr     \ cmp x0, x1
  here { adr_cond } reserve_instr  \ b.<cond> <end>

  adr_cond           >stack
  adr_beg            >stack
  lim                >stack
  cur                >stack
  step               >stack
  ' loop_countup_pop >stack
;

: loop_countdown_pop
  ( fun_prev …aux… adr_cond adr_beg loc_lim loc_cur loc_step )
  stack> stack> stack> { step cur lim }
  ASM_LT count_loop_pop \ b <beg>; retropatch b.<cond> <end>

  \ Don't need registers for these locals anymore.
  lim  comp_local_free
  step comp_local_free
;

\ Like in `+loop:`, the range is `[floor,ceil)`.
:: -loop:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg loc_cur loc_step fun_pop )
  ( E: ceil floor step -- )
  c" when calling `-loop:`" 3 comp_args_valid 0 comp_args_set
  loop_frame_init

  parse_word comp_named_local { cur } \ ceil  = cursor
  comp_anon_local             { lim } \ floor = limit
  comp_anon_local             { step }

  0 cur             comp_local_set \ mov <cur>,  x0 | str x0, [FP, <cur>]
  1 lim             comp_local_set \ mov <lim>,  x1 | str x1, [FP, <lim>]
  2 step            comp_local_set \ mov <step>, x2 | str x2, [FP, <step>]
  comp_barrier                     \ Clobber / relocate locals.
  here { adr_beg }                 \ Skip above instrs when repeating.
  0 cur             comp_local_get \ mov x0, <cur>  | ldr x0, [FP, <cur>]
  1 lim             comp_local_get \ mov x1, <lim>  | ldr x1, [FP, <lim>]
  2 step            comp_local_get \ mov x2, <step> | ldr x2, [FP, <step>]
  0 0 2 asm_sub_reg comp_instr     \ sub x0, x0, x2
  0 1   asm_cmp_reg comp_instr     \ cmp x0, x1
  here { adr_cond } reserve_instr  \ b.<cond> <end>
  0 cur             comp_local_set \ mov <cur>, x0 | str x0, [FP, <cur>]

  adr_cond             >stack
  adr_beg              >stack
  lim                  >stack
  cur                  >stack
  step                 >stack
  ' loop_countdown_pop >stack
;

\ ## Varargs and formatting

\ Short for "compile variadic arguments begin".
\ Assumes the Apple Arm64 ABI where varargs use the systack.
: comp_va_beg { len -- }
  len 2 align_up 0 2
  -loop: ind
    ind     { Xd }
    ind inc { Xt }
    \ We always store pairs because Arm64 requires the SP to be aligned to 16
    \ bytes when accessing memory. When length is odd, we also store an unused
    \ garbage value from an unused register, which is fine.
    Xd Xt ASM_REG_SP -16 asm_store_pair_pre comp_instr \ stp Xd, Xt, [SP, -16]!
  #end
;

\ Short for "compile variadic arguments end".
: comp_va_end { len -- }
  len >=0 #if
    len cells 16 align_up { off }
    ASM_REG_SP ASM_REG_SP off asm_add_imm comp_instr \ add sp, sp, <off>
  #end
;

\ For use in compile-time words; see `logf"` below.
: comp_va{ ( C: -- len ) ( E: <systack_push> )
  comp_args_get { len }
  len comp_va_beg
  len >stack
  0 comp_args_set
;

\ For use in compile-time words; see `logf"` below.
: }va_comp ( C: len -- ) ( E: <systack_pop ) stack> comp_va_end ;

\ Sets up arguments for a variadic call. Usage example:
\
\   : some_word
\     10 20 30 va{ c" numbers: %zd %zd %zd" printf }va lf
\   ;
\
\ Caution: varargs can only be used in direct calls to variadic procedures.
\ Indirect calls DO NOT WORK because the stack pointer is changed by calls.
:: va{ comp_va{ ;
:: }va }va_comp ;

\ Format-prints to stdout using `printf`. Usage example:
\
\   10 20 30 logf" numbers: %zd %zd %zd" lf
\
\ TODO define a `:` variant.
:: logf" ( C: i0 … iN -- ) ( E: i0 … iN -- )
  comp_va{ comp_cstr compile' printf }va_comp
;

\ Format-prints to stderr. TODO define a `:` variant.
:: elogf" ( C: N -- ) ( E: i1 … iN -- )
  comp_va{ compile' stderr comp_cstr compile' fprintf }va_comp
;

\ TODO: port `sf"` from stack-CC.

\ ## Exceptions — continued

4096 buf: ERR_BUF

\ Like `sthrowf"` but easier to use. Example:
\
\   10 20 30 throwf" error codes: %zd %zd %zd"
\
\ TODO: port `sthrowf"` from stack-CC.
:: throwf" ( C: len -- ) ( E: i1 … iN -- )
  comp_va{
    compile'  ERR_BUF
    comp_cstr
    compile'  snprintf
  }va_comp
  compile' ERR_BUF
  1 comp_args_set \ Drop buffer length.
  compile' throw
;

\ Similar to standard `abort"`, with clearer naming.
:: throw_if" ( C: "str" -- ) ( E: pred -- )
  execute'' #if
  comp_cstr
  compile' throw
  execute'' #end
;

\ ## Stack manipulation — continued

\ Variadic alternative to `>stack`.
:: >>stack ( E: …args… -- )
  ASM_REG_SCRATCH { top }

  top                comp_clobber
  top asm_stack_load comp_instr

  comp_args_get 0 +for: arg
    arg top 8 asm_store_post comp_instr \ str <arg>, [<top>], 8
  #end

  top asm_stack_store comp_instr
  0 comp_args_set
;

\ Variadic assignment of locals from the stack:
\
\   10 20 30 >>stack
\   stack{ ten twenty thirty }
:: stack{ ( E: …args… -- )
  0 { locs }

  #loop
    parse_word { str len }

    " }" str len str= #if #leave #end
    " --" str len str= throw_if" unsupported `--` in `stack{`"

    str len comp_named_local >stack
    locs inc { locs }
  #end

  locs #ifn #ret #end

  0 { arg_reg } \ x0
  1 { top_reg } \ x1

  arg_reg comp_clobber
  top_reg comp_clobber
  top_reg asm_stack_load comp_instr

  locs #for
    stack> { loc }
    arg_reg top_reg -8 asm_load_pre comp_instr     \ ldr x0, [x1, -8]!
    arg_reg loc                     comp_local_set \ mov <loc>, x0 | str x0, [FP, <loc>]
    arg_reg comp_clobber
  #end

  top_reg asm_stack_store comp_instr
;

\ ldur <stack>, [x27, INTERP_INTS_TOP]
\ ...
\ str  <arg>,   [<stack>], 8
\ ...
\ stur <stack>, [x27, INTERP_INTS_TOP]
: comp_args_to_stack { len }
  len #ifn #ret #end
  comp_scratch_reg { stack }

  stack ASM_REG_INTERP INTERP_INTS_TOP asm_load_off comp_instr \ ldur <stack>
  len 0 +for: arg
    arg stack 8 asm_store_post comp_instr \ str <arg>, [<stack>], 8
  #end
  stack ASM_REG_INTERP INTERP_INTS_TOP asm_store_off comp_instr \ stur <stack>
;

\ ## Stack printing

: log_int  { num     -- } num logf" %zd" ;
: log_cell { num ind -- } num num ind logf" %zd 0x%zx <%zd>" ;
: .        { num     -- } stack_len { ind } num ind log_cell lf ;

: .s
  stack_len { len }

  len #ifn
    log" stack is empty" lf
    #ret
  #end

  len <0 #if
    len logf" stack length is negative: %zd" lf
    #ret
  #end

  len logf" stack <%zd>:" lf

  len 0 +for: ind
    space space
    ind stack_at @ ind log_cell lf
  #end
;

: .sc .s stack_clear ;

\ ## More memory stuff

extern_val: errno __error
1 1 extern: strerror

6 1 extern: mmap     ( adr len pflag mflag fd off -- adr )
3 1 extern: mprotect ( adr len pflag -- err )

16384 let: PAGE_SIZE \ TODO: ask the OS.
0     let: PROT_NONE
1     let: PROT_READ
2     let: PROT_WRITE
2     let: MAP_PRIVATE
4096  let: MAP_ANON

: mem_map_err
  errno        { err }
  err strerror { str }
  err str throwf" unable to map memory; code: %d; message: %s"
;

: mem_map { size pflag -- addr }
  MAP_ANON MAP_PRIVATE or { mflag }
  0 size pflag mflag -1 0 mmap { addr }
  addr -1 = #if mem_map_err #end
  addr
;

: mem_unprot_err
  errno        { err }
  err strerror { str }
  err str throwf" unable to unprotect memory; code: %d; message: %s"
;

: mem_unprot { addr size }
  PROT_READ PROT_WRITE or { pflag }
  addr size pflag mprotect
  -1 = #if throw" unable to mprotect" #end
;

\ Allocates a guarded buffer: `guard|data|guard`.
\ Attempting to read or write inside the guards,
\ aka underflow or overflow, triggers a segfault.
\ The given size is rounded up to the page size.
: mem_alloc { size1 -- addr size2 }
  size1 PAGE_SIZE align_up { size2 }
  PAGE_SIZE 1 lsl size2 +  { size }
  size PROT_NONE mem_map   { addr }
  addr PAGE_SIZE +         { addr }
  addr size2 mem_unprot
  addr size2
;

\ Allocates space for N cells with lower and upper guards. Underflowing and
\ overflowing into the guards triggers a segfault. The size gets page-aligned,
\ usually leaving additional unguarded readable and writable space between the
\ requested region and the upper guard. The lower guard is immediately below
\ the returned address and immediately prevents underflows.
\
\ TODO consider compiling with lazy-init; dig up the old code.
: cells_guard: { len } ( C: "name" -- ) ( E: -- adr )
  parse_word          { name name_len }
  len cells mem_alloc { adr -- }
  name name_len adr init_data_word
  [ not_comp_only ]
;

\ ## Pretend types
\
\ We don't have a real type system, but it can still be useful
\ to use type names, which simply return sizes, similar to the
\ existing practice of `cell` / `cells`.

\ Sizeof of various numbers. Assumes Arm64 + Apple ABI.
1 let: S8
1 let: U8
2 let: S16
2 let: U16
4 let: S32
4 let: U32
8 let: S64
8 let: U64
8 let: Adr
8 let: Cstr \ char*
4 let: Cint
4 let: Cuint

\ Defines a pretend "array type" which is really just capacity.
\ Intended mainly for making struct fields more descriptive.
\ See structs below. Usage:
\
\   123      let: SIZE
\   234      let: LEN
\   SIZE LEN arr: Type
\   Type     mem: BUF
: arr: { size len -- } ( C: "name" -- ) ( E: -- cap )
  size len * execute' let:
;

\ ## Pretend struct types
\
\ Basic usage:
\
\   struct: Type
\     S32 field: Type_field0
\     U64 field: Type_field1
\   #end
\
\   Type calloc { val }
\
\       val Type_field1 @
\   123 val Type_field1 !

: struct_pop ( C: str len off -- )
  stack{ str len off }
  str len off let_init
  str free \ String was `strdup`'d.
  [ not_comp_only ]
;

: struct: { -- str len off fun } ( C: "name" -- ) ( E: -- size )
  parse_word   { str len }
  str strdup   { str }
  ' struct_pop { fun }
  str len 0 fun
;

: struct_field_comp { off }
  comp_args_get { len }
  len #ifn throw" struct field must be preceded by struct pointer" #end

  len dec { reg }
  reg                     comp_clobber \ Disassociate locals from this reg.
  reg reg off asm_add_imm comp_instr   \ add <reg>, <reg>, <off>
;

\ Since structs are intended for C interop, field alignment
\ must match the C struct ABI of the target platform.
: field: { off fun size -- off_next fun }
  ( C: "name" -- )
  ( E: struct_adr -- field_adr )
  off size align_up { field_off }
  field_off size +  { next_off }
  parse_word        { str len }

  \ Execution-time semantics.
  str len colon
    1 1 comp_signature ( E: struct_adr -- field_adr )
    1 comp_args_set
    field_off comp_push
    compile' +
  semicolon

  \ Compile-time semantics.
  str len colon_colon
    field_off comp_push
    compile' struct_field_comp
  semicolon

  next_off fun
  [ not_comp_only ]
;
