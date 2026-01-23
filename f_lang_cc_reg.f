:: \ 10 parse { -- } ;
:: ( 41 parse { -- } ;

\ Can use comments now!
\
\ This file boostraps the Forth language. The outer interpreter / compiler
\ provides only the most fundamental intrinsics; enough for self-assembly.
\ We define basic words in terms of machine instructions, building up from
\ there. Currently only the Arm64 CPU architecture is supported.
\
\ The reason for `::` will become apparent later.

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
: comp_push { val -- } ( E: -- val ) comp_next_arg_reg { reg } val reg comp_load ;

: next_word { list -- XT } ( "word" -- XT ) parse_word list find_word ;

:  tick_next { list -- } ( "word" -- ) ( E: -- XT ) list next_word comp_push ;
:: '  1 tick_next ; \ WORDLIST_EXEC
:: '' 2 tick_next ; \ WORDLIST_COMP

:  inline_next { list -- } ( "word" -- ) ( E: <word> )
  list next_word inline_word
;
:: inline'  1 inline_next ;
:: inline'' 2 inline_next ;

\ "execute" is renamed from standard "postpone".
:  execute_next { list -- } ( "word" -- ) ( E: <word> )
  list next_word comp_call
;
:: execute'  1 execute_next ;
:: execute'' 2 execute_next ;

:  compile_next { list -- } ( "word" -- )
  list next_word comp_push ' comp_call comp_call
;
:: compile'  1 compile_next ;
:: compile'' 2 compile_next ;

: semicolon execute'' ; ;

: @ { adr -- val } [
  comp_next_arg
  0b11_111_0_00_01_0_000000000_00_00000_00000 comp_instr \ ldur x0, [x0]
] ;

: ! { val adr -- } [
  0b11_111_0_00_00_0_000000000_00_00001_00000 comp_instr \ stur x0, [x1]
] ;

\ Similar to standard `constant`.
: let: { val -- } ( C: "name" -- ) ( E: -- val )
  parse_word { str len }

  \ Execution-time semantics.
  str len colon
    0 1 comp_word_sig ( E: -- val )
    val comp_push
  semicolon

  \ Compile-time semantics.
  str len colon_colon
    val comp_push compile' comp_push
  semicolon

  [ not_comp_only ]
;

8 let: cell

\ ## Assembler and bitwise ops
\
\ We interleave definitions of assembler words with definitions
\ of various arithmetic words used by the assembler.

4      let: ASM_INSTR_SIZE
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

: or { i1 i2 -- i3 } [
  comp_next_arg
  0b1_01_01010_00_0_00001_000000_00000_00000 comp_instr \ orr x0, x0, x1
] ;

: and { i1 i2 -- i3 } [
  comp_next_arg
  0b1_00_01010_00_0_00001_000000_00000_00000 comp_instr \ and x0, x0, x1
] ;

: xor { i1 i2 -- i3 } [
  comp_next_arg
  0b1_10_01010_00_0_00001_000000_00000_00000 comp_instr \ eor x0, x0, x1
] ;

: lsl { i1 bits -- i2 } [
  comp_next_arg
  0b1_0_0_11010110_00001_0010_00_00000_00000 comp_instr \ lsl x0, x0, x1
] ;

: lsr { i1 bits -- i2 } [
  comp_next_arg
  0b1_0_0_11010110_00001_0010_01_00000_00000 comp_instr \ lsr x0, x0, x1
] ;

: invert { i1 -- i2 } [
  comp_next_arg
  0b1_01_01010_00_1_00000_000000_11111_00000 comp_instr \ mvn x0, x0
] ;

\ Our parsing rules prevent `1+` or `+1` from being a word name.
: inc { i1 -- i2 } [
  comp_next_arg
  0b1_0_0_100010_0_000000000001_00000_00000 comp_instr \ add x0, x0, 1
] ;

\ Our parsing rules prevent `1-` or `-1` from being a word name.
: dec { i1 -- i2 } [
  comp_next_arg
  0b1_1_0_100010_0_000000000001_00000_00000 comp_instr \ sub x0, x0, 1
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
  comp_next_arg
  0        asm_cmp_zero comp_instr \ cmp x0, 0
  0 ASM_NE asm_cset     comp_instr \ cset x0, ne
] ;

: =0 { int -- bool } [
  comp_next_arg
  0        asm_cmp_zero comp_instr \ cmp x0, 0
  0 ASM_EQ asm_cset     comp_instr \ cset x0, eq
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
: asm_mul { Xd Xn imm12 -- instr }
  Xd Xn imm12 asm_pattern_arith_imm
  0b1_00_11011_000_00000_0_11111_00000_00000 or
;

\ sdiv Xd, Xn, Xm
: asm_sdiv { Xd Xn imm12 -- instr }
  Xd Xn imm12 asm_pattern_arith_imm
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
: asm_asr_reg { Xd Xn imm12 -- instr }
  Xd Xn imm12 asm_pattern_arith_imm
  0b1_0_0_11010110_00000_0010_10_00000_00000 or
;

\ Arithmetic (sign-preserving) right shift.
: asr { i1 bits -- i2 } [
  comp_next_arg
  0 0 1 asm_asr_reg comp_instr \ asr x0, x0, x1
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
: asm_branch { imm26 -- instr }
  imm26
  2  lsr       \ Offset is implicitly times 4.
  26 bit_trunc \ Offset may be negative.
  0b0_00_101_00000000000000000000000000 or
;

\ b.cond <off>
: asm_branch_cond { cond off -- instr }
  off 2 asr 19 bit_trunc 5 lsl \ Implicitly times 4.
  cond or
  0b010_101_00_0000000000000000000_0_0000 or
;

\ `imm19` offset is implicitly times 4 and can be negative.
: asm_pattern_compare_branch { Xt imm19 -- instr_mask }
  imm19 2 lsr 19 bit_trunc 5 lsl
  Xt or
;

\ cbz x1, <off>
: asm_cmp_branch_zero { Xt imm19 -- instr }
  Xt imm19 asm_pattern_compare_branch
  0b1_011010_0_0000000000000000000_00000 or
;

\ cbnz x1, <off>
: asm_cmp_branch_not_zero { Xt imm19 -- instr }
  Xt imm19 asm_pattern_compare_branch
  0b1_011010_1_0000000000000000000_00000 or
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

: negate { i1 -- i2 } [
  comp_next_arg
  0 asm_neg comp_instr \ neg x0, x0
] ;

: + { i1 i2 -- i3 } [
  comp_next_arg
  0 0 1 asm_add_reg comp_instr \ add x0, x0, x1
] ;

: - { i1 i2 -- i3 } [
  comp_next_arg
  0 0 1 asm_sub_reg comp_instr \ sub x0, x0, x1
] ;

: * { i1 i2 -- i3 } [
  comp_next_arg
  0 0 1 asm_mul comp_instr \ mul x0, x0, x1
] ;

: / { i1 i2 -- i3 } [
  comp_next_arg
  0 0 1 asm_sdiv comp_instr \ sdiv x0, x0, x1
] ;

\ FIXME: declare clobber
: mod { i1 i2 -- i3 } [
    2 0 1 asm_sdiv comp_instr \ sdiv x2, x0, x1
  0 2 1 0 asm_msub comp_instr \ msub x0, x2, x1, x0
  comp_next_arg
] ;

\ FIXME: declare clobber
: /mod { dividend divisor -- rem quo } [
  comp_next_arg comp_next_arg
    2 0 1 asm_sdiv  comp_instr \ sdiv x2, x0, x1
  0 2 1 0 asm_msub  comp_instr \ msub x0, x2, x1, x0
] ;

: abs { int -- +int } [
  comp_next_arg
  0            asm_cmp_zero comp_instr \ cmp x0, 0
  0 0 0 ASM_PL asm_csneg    comp_instr \ cneg x0, x0, pl
] ;

: min { i1 i2 -- i1|i2 } [
  comp_next_arg
  0 1          asm_cmp_reg comp_instr \ cmp x0, x1
  0 0 1 ASM_LT asm_csel    comp_instr \ csel x0, x0, x1, lt
] ;

: max { i1 i2 -- i1|i2 } [
  comp_next_arg
  0 1          asm_cmp_reg comp_instr \ cmp x0, x1
  0 0 1 ASM_GT asm_csel    comp_instr \ csel x0, x0, x1, gt
] ;

: align_down { len wid -- len } wid negate len and ;
: align_up   { len wid -- len } wid dec len + wid align_down ;

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

: asm_comp_cset_reg { cond -- } ( E: i1 i2 -- bool )
  comp_next_arg
  0 1    asm_cmp_reg comp_instr \ cmp x0, x1
  0 cond asm_cset    comp_instr \ cset x0, <cond>
;

: asm_comp_cset_zero { cond -- } ( E: num -- bool )
  comp_next_arg
  0      asm_cmp_zero comp_instr \ cmp x0, 0
  0 cond asm_cset     comp_instr \ cset x0, <cond>
;

\ ## Numeric comparison

\ https://gforth.org/manual/Numeric-comparison.html
: =   { i1 i2 -- bool } [ ASM_EQ asm_comp_cset_reg  ] ;
: <>  { i1 i2 -- bool } [ ASM_NE asm_comp_cset_reg  ] ;
: >   { i1 i2 -- bool } [ ASM_GT asm_comp_cset_reg  ] ;
: <   { i1 i2 -- bool } [ ASM_LT asm_comp_cset_reg  ] ;
: <=  { i1 i2 -- bool } [ ASM_LE asm_comp_cset_reg  ] ;
: >=  { i1 i2 -- bool } [ ASM_GE asm_comp_cset_reg  ] ;
: <0  { num   -- bool } [ ASM_LT asm_comp_cset_zero ] ; \ Or `MI`.
: >0  { num   -- bool } [ ASM_GT asm_comp_cset_zero ] ;
: <=0 { num   -- bool } [ ASM_LE asm_comp_cset_zero ] ;
: >=0 { num   -- bool } [ ASM_GE asm_comp_cset_zero ] ; \ Or `PL`.

: odd { i1 -- bool } i1 1 and ;

\ ## Characters and strings

: c@ { str -- char } [
  comp_next_arg
  0 0 0 asm_load_byte_off comp_instr \ ldrb x0, [x0]
] ;

: parse_char { -- char } parse_word { buf -- } buf c@ ;

: char' { -- char } parse_char ;

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

:  " { -- cstr len }                    parse_str ;
:: " ( C: "str" -- ) ( E: -- cstr len ) comp_str ;

:  c" { -- cstr }                    parse_cstr ;
:: c" ( C: "str" -- ) ( E: -- cstr ) comp_cstr ;

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

1 1 extern: strlen  ( cstr -- len )
3 1 extern: strncmp ( str0 str1 len -- )
3 0 extern: strncpy ( buf[tar] buf[src] len -- )
3 0 extern: strlcpy ( buf[tar] buf[src] buf_len -- )

\ FIXME finish
\
\ : move    ( buf[src] buf[tar] len -- ) swap_over strncpy ;
\ : fill    ( buf len char -- ) swap memset ;
\ : erase   ( buf len -- ) 0 fill ;
\ : blank   ( buf len -- ) 32 fill ;
\ : compare ( str0 len0 str1 len1 -- direction ) rot min strncmp ;
\ : str=    ( str0 len0 str1 len1 -- bool ) compare =0 ;
\ : str<    ( str0 len0 str1 len1 -- bool ) compare <0 ;
\ : str<>   ( str0 len0 str1 len1 -- bool ) compare <>0 ;

\ ## Variables, buffers, extern vars

\ Asks for a register and compiles this idiom:
\
\   adrp <reg>, <page>
\   add <reg>, <reg>, <pageoff>
\
\ Used for taking an address of a data entry
\ in the code heap allocated via `alloc_data`.
: comp_extern_addr { adr -- } ( E: -- adr )
  comp_next_arg_reg { reg }
  adr reg comp_page_addr
;

\ Used for loading a value in a data entry
\ or GOT entry in the code heap.
: comp_extern_load { adr -- } ( E: -- val )
  comp_next_arg_reg { reg }
  adr reg comp_page_load \ adrp & ldr
;

\ Used for loading the value behind an address
\ stored in a GOT entry in the code heap.
: comp_extern_load_load { adr -- } ( E: -- val )
  comp_next_arg_reg { reg }
  adr reg                comp_page_load \ adrp & ldr
  reg reg 0 asm_load_off comp_instr     \ ldur x0, [x0]
;

\ Similar to standard `variable`, but takes an initial value like `let:`.
: var: { val -- } ( C: "name" -- ) ( E: -- ptr )
  parse_word { str len }
  0 cell alloc_data { adr }
  val adr !

  str len colon
    0 1 comp_word_sig ( E: -- ptr )
    adr comp_extern_addr
  semicolon

  str len colon_colon
    adr comp_push compile' comp_extern_addr
  semicolon

  [ not_comp_only ]
;

: comp_buf { adr cap -- } ( E: -- adr cap )
  comp_next_arg_reg { RA }
  comp_next_arg_reg { RL }
  adr RA comp_page_addr
  cap RL comp_load
;

\ Similar to the standard idiom `create <name> N allot`.
\ Creates a global variable which refers to a buffer of
\ at least the given size in bytes.
: buf: { cap -- } ( C: cap "name" -- ) ( E: -- adr cap )
  parse_word { str len }
  0 cap alloc_data { adr }

  str len colon
    0 2 comp_word_sig ( E: -- ptr )
    adr cap comp_buf
  semicolon

  str len colon_colon
    adr comp_push cap comp_push compile' comp_buf
  semicolon

  [ not_comp_only ]
;

4096 buf: BUF

\ Analogous to `extern:`, but for external variables rather than procedures.
: extern_val: ( C: "ours" "extern" -- ) ( E: -- val )
  parse_word { str len }

  \ Get a stable name location because we're about
  \ to parse one more word, which overwrites `str`.
  BUF { buf cap } buf str cap strlcpy

  parse_word extern_got { adr }

  buf len colon
    0 1 comp_word_sig ( E: -- val )
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

: eputchar { char -- } char stderr fputc ;
: puts { cstr -- } cstr stdout fputs ;
: eputs { cstr -- } cstr stderr fputs ;
: flush stdout fflush ;
: eflush stderr fflush ;
: type { str len -- } str 1 len stdout fwrite ;
: etype { str len -- } str 1 len stderr fwrite ;
: lf 10 putchar ; \ Renamed from `cr` which would be a misnomer.
: elf 10 eputchar ;
: space 32 putchar ;
: espace 32 eputchar ;

:  log" parse_cstr puts ;
:: log" comp_cstr compile' puts ;

:  elog" parse_cstr eputs ;
:: elog" comp_cstr compile' eputs ;

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

: mem_map { len pflag -- adr }
  MAP_ANON MAP_PRIVATE or { mflag }
  0 len pflag mflag -1 0 mmap
;

: mem_unprot { adr len -- err }
  PROT_READ PROT_WRITE or { pflag }
  adr len pflag mprotect
;

: mem_alloc { size1 -- addr size2 }
  size1 PAGE_SIZE align_up { size2 }
  PAGE_SIZE 1 lsl size2 +  { size }
  size PROT_NONE mem_map   { addr }
  addr PAGE_SIZE +         { addr }
  addr size2 mem_unprot    { -- }
  addr size2
;

debug_stack
