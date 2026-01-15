: drop2 [ 0b1_1_0_100010_0_000000010000_11011_11011 comp_instr ] ;
: \ [ immediate ] 10 parse drop2 ;
: ( [ immediate ] 41 parse drop2 ;

(
Can use comments now!

This file boostraps the Forth language. The outer interpreter / compiler
provides only the most fundamental intrinsics; enough for self-assembly.
We define basic words in terms of machine instructions, building up from
there. Currently only the Arm64 CPU architecture is supported.
)

\ brk 666
: abort [ 0b110_101_00_001_0000001010011010_000_00 comp_instr ] ;
: unreachable abort ;

: ASM_EQ 0b0000 ;
: ASM_NE 0b0001 ;
: ASM_CS 0b0010 ;
: ASM_CC 0b0011 ;
: ASM_MI 0b0100 ;
: ASM_PL 0b0101 ;
: ASM_VS 0b0110 ;
: ASM_VC 0b0111 ;
: ASM_HI 0b1000 ;
: ASM_LS 0b1001 ;
: ASM_GE 0b1010 ;
: ASM_LT 0b1011 ;
: ASM_GT 0b1100 ;
: ASM_LE 0b1101 ;
: ASM_AL 0b1110 ;
: ASM_NV 0b1111 ;

: asm_pop_x1_x2
  \ ldp x1, x2, [x27, -16]!
  0b10_101_0_011_1_1111110_00010_11011_00001
;

: asm_push_x1_x2
  \ stp x1, x2, [x27], 16
  0b10_101_0_001_0_0000010_00010_11011_00001
;

: asm_pop_x1
  \ ldr x1, [x27, -8]!
  0b11_111_0_00_01_0_111111000_11_11011_00001
;

: asm_push_x1
  \ str x1, [x27], 8
  0b11_111_0_00_00_0_000001000_01_11011_00001
;

: or ( i1 i2 -- i3 ) [
  asm_pop_x1_x2                              comp_instr \ ldp x1, x2, [x27, -16]!
  0b1_01_01010_00_0_00010_000000_00001_00001 comp_instr \ orr x1, x1, x2
  asm_push_x1                                comp_instr \ str x1, [x27], 8
] ;

: and ( i1 i2 -- i3 ) [
  asm_pop_x1_x2                              comp_instr \ ldp x1, x2, [x27, -16]!
  0b1_00_01010_00_0_00010_000000_00001_00001 comp_instr \ and x1, x1, x2
  asm_push_x1                                comp_instr \ str x1, [x27], 8
] ;

: xor ( i1 i2 -- i3 ) [
  asm_pop_x1_x2                              comp_instr \ ldp x1, x2, [x27, -16]!
  0b1_10_01010_00_0_00010_000000_00001_00001 comp_instr \ eor x1, x1, x2
  asm_push_x1                                comp_instr \ str x1, [x27], 8
] ;

: lsl ( i1 bits -- i2 ) [
  asm_pop_x1_x2                              comp_instr \ ldp x1, x2, [x27, -16]!
  0b1_0_0_11010110_00010_0010_00_00001_00001 comp_instr \ lsl x1, x1, x2
  asm_push_x1                                comp_instr \ str x1, [x27], 8
] ;

: lsr ( i1 bits -- i2 ) [
  asm_pop_x1_x2                              comp_instr \ ldp x1, x2, [x27, -16]!
  0b1_0_0_11010110_00010_0010_01_00001_00001 comp_instr \ lsr x1, x1, x2
  asm_push_x1                                comp_instr \ str x1, [x27], 8
] ;

: invert ( i1 -- i2 ) [
  asm_pop_x1                                 comp_instr \ ldr x1, [x27, -8]!
  0b1_01_01010_00_1_00001_000000_11111_00001 comp_instr \ mvn x1, x1
  asm_push_x1                                comp_instr \ str x1, [x27], 8
] ;

\ Our parsing rules prevent `1+` or `+1` from being a word name.
: inc ( i1 -- i2 ) [
  asm_pop_x1                                comp_instr \ ldr x1, [x27, -8]!
  0b1_0_0_100010_0_000000000001_00001_00001 comp_instr \ add x1, x1, 1
  asm_push_x1                               comp_instr \ str x1, [x27], 8
] ;

\ Our parsing rules prevent `1-` or `-1` from being a word name.
: dec ( i1 -- i2 ) [
  asm_pop_x1                                comp_instr \ ldr x1, [x27, -8]!
  0b1_1_0_100010_0_000000000001_00001_00001 comp_instr \ sub x1, x1, 1
  asm_push_x1                               comp_instr \ str x1, [x27], 8
] ;

: swap ( i1 i2 -- i2 i1 ) [
  asm_pop_x1_x2                              comp_instr \ ldp x1, x2, [x27, -16]!
  0b10_101_0_001_0_0000010_00001_11011_00010 comp_instr \ stp x2, x1, [x27], 16
] ;

: low_bits ( bit_len -- bits ) 1 swap lsl dec ;
: bit_trunc ( imm bit_len -- imm ) low_bits and ;

\ cmp Xn, 0
: asm_cmp_zero ( Xn -- instr )
  5 lsl
  0b1_1_1_100010_0_000000000000_00000_11111
  or
;

: asm_cmp_imm ( Xn imm -- instr )
       10 lsl    \ imm
  swap 5  lsl or \ Xn
  0b1_1_1_100010_0_000000000000_00000_11111 or
;

\ cset Xd, <cond>
: asm_cset ( Xd cond -- )
  0b1_0_0_11010100_11111_0000_0_1_11111_00000
  swap 1 xor 12 lsl or \ cond
                    or \ Xd
;

: <>0 ( int -- bool ) [
           asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
         1 asm_cmp_zero comp_instr \ cmp x1, 0
  1 ASM_NE asm_cset     comp_instr \ cset x1, ne
           asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: =0 ( int -- bool ) [
           asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
         1 asm_cmp_zero comp_instr \ cmp x1, 0
  1 ASM_EQ asm_cset     comp_instr \ cset x1, eq
           asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

\ Shared by a lot of load and store instructions. 64-bit only.
: asm_pattern_load_store_pair ( Xt1 Xt2 Xn imm7 base_instr -- instr )
  swap 3 lsr 7 bit_trunc 15 lsl or \ imm7
  swap                   5  lsl or \ Xn
  swap                   10 lsl or \ Xt2
                                or \ Xt1
;

\ ldp Xt1, Xt2, [Xn, <imm>]!
: asm_load_pair_pre ( Xt1 Xt2 Xn imm7 -- instr )
  0b10_101_0_011_1_0000000_00000_00000_00000
  asm_pattern_load_store_pair
;

\ ldp Xt1, Xt2, [Xn], <imm>
: asm_load_pair_post ( Xt1 Xt2 Xn imm7 -- instr )
  0b10_101_0_001_1_0000000_00000_00000_00000
  asm_pattern_load_store_pair
;

\ ldp Xt1, Xt2, [Xn, <imm>]
: asm_load_pair_off ( Xt1 Xt2 Xn imm7 -- instr )
  0b10_101_0_010_1_0000000_00000_00000_00000
  asm_pattern_load_store_pair
;

\ stp Xt1, Xt2, [Xn, <imm>]!
: asm_store_pair_pre ( Xt1 Xt2 Xn imm7 -- instr )
  0b10_101_0_011_0_0000000_00000_00000_00000
  asm_pattern_load_store_pair
;

\ stp Xt1, Xt2, [Xn], <imm>
: asm_store_pair_post ( Xt1 Xt2 Xn imm7 -- instr )
  0b10_101_0_001_0_0000000_00000_00000_00000
  asm_pattern_load_store_pair
;

\ stp Xt1, Xt2, [Xn, <imm>]
: asm_store_pair_off ( Xt1 Xt2 Xn imm7 -- instr )
  0b10_101_0_010_0_0000000_00000_00000_00000
  asm_pattern_load_store_pair
;

(
Shared by a lot of load and store instructions.
Patches these bits:
  0b00_000_0_00_00_0_XXXXXXXXX_00_XXXXX_XXXXX
)
: asm_pattern_load_store ( Xt Xn imm9 base_instr -- instr )
  swap 9 bit_trunc 12 lsl or \ imm9
  swap             5  lsl or \ Xn
                          or \ Xt
;

\ ldr Xt, [Xn, <imm>]!
: asm_load_pre ( Xt Xn imm9 -- instr )
  0b11_111_0_00_01_0_000000000_11_00000_00000
  asm_pattern_load_store
;

\ ldr Xt, [Xn], <imm>
: asm_load_post ( Xt Xn imm9 -- instr )
  0b11_111_0_00_01_0_000000000_01_00000_00000
  asm_pattern_load_store
;

\ str Xt, [Xn, <imm>]!
: asm_store_pre ( Xt Xn imm9 -- instr )
  0b11_111_0_00_00_0_000000000_11_00000_00000
  asm_pattern_load_store
;

\ str Xt, [Xn], <imm>
: asm_store_post ( Xt Xn imm9 -- instr )
  0b11_111_0_00_00_0_000000000_01_00000_00000
  asm_pattern_load_store
;

\ ldur Xt, [Xn, <imm>]
: asm_load_off ( Xt Xn imm9 -- instr )
  0b11_111_0_00_01_0_000000000_00_00000_00000
  asm_pattern_load_store
;

\ stur Xt, [Xn, <imm>]
: asm_store_off ( Xt Xn imm9 -- instr )
  0b11_111_0_00_00_0_000000000_00_00000_00000
  asm_pattern_load_store
;

\ ldur Wt, [Xn, <imm>]
: asm_load_off_32 ( Wt Xn imm9 -- instr )
  0b10_111_0_00_01_0_000000000_00_00000_00000
  asm_pattern_load_store
;

\ stur Wt, [Xn, <imm>]
: asm_store_off_32 ( Wt Xn imm9 -- instr )
  0b10_111_0_00_00_0_000000000_00_00000_00000
  asm_pattern_load_store
;

(
Shared by register-offset load and store instructions.
`scale` must be 0 or 1; if 1, offset is multiplied by 8.
Assumes `lsl` is already part of the base instruction.
)
: asm_pattern_load_store_with_register ( Xt Xn Xm scale base_instr -- instr )
  swap <>0 12 lsl or \ lsl 3
  swap     16 lsl or \ Xm
  swap     5  lsl or \ Xn
                  or \ Xt
;

\ ldr Xt, [Xn, Xm, lsl <scale>]
: asm_load_with_register ( Xt Xn Xm scale -- instr )
  0b11_111_0_00_01_1_00000_011_0_10_00000_00000
  asm_pattern_load_store_with_register
;

\ str Xt, [Xn, Xm, lsl <scale>]
: asm_store_with_register ( Xt Xn Xm scale -- instr )
  0b11_111_0_00_00_1_00000_011_0_10_00000_00000
  asm_pattern_load_store_with_register
;

\ Shared by some integer arithmetic instructions.
: asm_pattern_arith_reg ( Xd Xn Xm -- instr_mask )
       16 lsl    \ Xm
  swap 5  lsl or \ Xn
              or \ Xd
;

\ Shared by some integer arithmetic and load/store instructions.
: asm_pattern_arith_imm ( Xd Xn imm12 -- instr_mask )
  12 bit_trunc 10 lsl    \ imm12
  swap          5 lsl or \ Xn
                      or \ Xd
;

: asm_load_byte_off ( Wt Wn imm12 -- instr )
  asm_pattern_arith_imm
  0b00_11_1_0_0_1_01_000000000000_00000_00000 or
;

: asm_store_byte_off ( Wt Wn imm12 -- instr )
  asm_pattern_arith_imm
  0b00_11_1_0_0_1_00_000000000000_00000_00000 or
;

\ str Xd, [x27], 8
: asm_push1 ( Xd -- instr ) 27 8 asm_store_post ;

\ ldr Xd, [x27, -8]!
: asm_pop1 ( Xd -- instr ) 27 -8 asm_load_pre ;

\ stp Xt1, Xt2, [x27], 16
: asm_push2 ( Xt1 Xt2 -- instr ) 27 16 asm_store_pair_post ;

\ ldp Xt1, Xt2, [x27, -16]!
: asm_pop2 ( Xt1 Xt2 -- instr ) 27 -16 asm_load_pair_pre ;

: asm_neg ( reg -- instr )
  0b1_1_0_01011_00_0_00001_000000_11111_00000 or
;

\ add Xd, Xn, <imm12>
: asm_add_imm ( Xd Xn imm12 -- instr )
  asm_pattern_arith_imm
  0b1_0_0_100010_0_000000000000_00000_00000 or
;

\ add Xd, Xn, Xm
: asm_add_reg ( Xd Xn Xm -- instr )
  asm_pattern_arith_reg
  0b1_0_0_01011_00_0_00000_000000_00000_00000 or
;

\ sub Xd, Xn, <imm12>
: asm_sub_imm ( Xd Xn imm12 -- instr )
  asm_pattern_arith_imm
  0b1_1_0_100010_0_000000000000_00000_00000 or
;

\ sub Xd, Xn, Xm
: asm_sub_reg ( Xd Xn Xm -- instr )
  asm_pattern_arith_reg
  0b1_1_0_01011_00_0_00000_000000_00000_00000 or
;

\ mul Xd, Xn, Xm
: asm_mul ( Xd Xn Xm -- instr )
  asm_pattern_arith_reg
  0b1_00_11011_000_00000_0_11111_00000_00000 or
;

\ sdiv Xd, Xn, Xm
: asm_sdiv ( Xd Xn Xm -- instr )
  asm_pattern_arith_reg
  0b1_0_0_11010110_00000_00001_1_00000_00000 or
;

\ msub Xd, Xn, Xm, Xa
: asm_msub ( Xd Xn Xm Xa -- instr )
  0b1_00_11011_000_00000_1_00000_00000_00000
  swap 10 lsl or \ Xa
  swap 16 lsl or \ Xm
  swap 5  lsl or \ Xn
              or \ Xd
;

: asm_csneg ( Xd Xn Xm cond -- instr )
  [ 3 asm_pop1 comp_instr ]  \ Stash `cond`.
  asm_pattern_arith_reg      \ Xd Xm Xn
  [ 3 asm_push1 comp_instr ] \ Pop `cond`.
  12 lsl or                  \ cond
  0b1_1_0_11010100_00000_0000_0_1_00000_00000 or
;

\ asr Xd, Xn, imm6
: asm_asr_imm ( Xd Xn imm6 -- instr )
  asm_pattern_arith_reg
  0b1_00_100110_1_000000_111111_00000_00000 or
;

\ asr Xd, Xn, Xm
: asm_asr_reg ( Xd Xn Xm -- instr )
  asm_pattern_arith_reg
  0b1_0_0_11010110_00000_0010_10_00000_00000 or
;

\ Arithmetic (sign-preserving) right shift.
: asr ( i1 bits -- i2 ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 1 2 asm_asr_reg   comp_instr \ asr x1, x1, x2
        asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

\ eor Xd, Xn, Xm
: asm_eor_reg ( Xd Xn Xm -- instr )
  asm_pattern_arith_reg
  0b1_10_01010_00_0_00000_000000_00000_00000 or
;

: asm_csel ( Xd Xn Xm cond -- instr )
  [ 3 asm_pop1 comp_instr ]  \ Stash `cond`.
  asm_pattern_arith_reg      \ Xd Xm Xn
  [ 3 asm_push1 comp_instr ] \ Pop `cond`.
  12 lsl or                  \ cond
  0b1_0_0_11010100_00000_0000_0_0_00000_00000 or
;

\ cmp Xn, Xm
: asm_cmp_reg ( Xn Xm -- instr )
       16 lsl    \ Xm
  swap 5  lsl or \ Xn
  0b1_1_1_01011_00_0_00000_000_000_00000_11111 or
;

\ b <off>
: asm_branch ( imm26 -- instr )
  2  lsr       \ Offset is implicitly times 4.
  26 bit_trunc \ Offset may be negative.
  0b0_00_101_00000000000000000000000000 or
;

\ b.cond <off>
: asm_branch_cond ( cond off -- instr )
  2 asr 19 bit_trunc 5 lsl \ `off`; implicitly times 4.
  or                       \ cond
  0b010_101_00_0000000000000000000_0_0000 or
;

\ `imm19` offset is implicitly times 4 and can be negative.
: asm_pattern_compare_branch ( Xt imm19 base_instr -- instr )
  swap 2 lsr 19 bit_trunc 5 lsl \ imm19
                             or \ instr
                             or \ Xt
;

\ cbz x1, <off>
: asm_cmp_branch_zero ( Xt imm19 -- instr )
  0b1_011010_0_0000000000000000000_00000
  asm_pattern_compare_branch
;

\ cbnz x1, <off>
: asm_cmp_branch_not_zero ( Xt imm19 -- instr )
  0b1_011010_1_0000000000000000000_00000
  asm_pattern_compare_branch
;

\ Same as `0 pick`.
: dup ( i1 -- i1 i1 ) [
  1 27 -8 asm_load_off comp_instr \ ldur x1, [x27, -8]
          asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

\ eor <reg>, <reg>, <reg>
: asm_zero_reg ( reg -- instr ) dup dup asm_eor_reg ;

: asm_mov_reg ( Xd Xm -- instr )
  0b1_01_01010_00_0_00000_000000_11111_00000
  swap 16 lsl or \ Xm
              or \ Xd
;

\ Immediate must be unsigned.
: asm_mov_imm ( Xd imm -- instr )
  0b1_10_100101_00_0000000000000000_00000
  swap 5 lsl or \ imm
             or \ Xd
;

\ mvn Xd, Xm
: asm_mvn ( Xd Xm -- instr )
  16 lsl \ Xm
      or \ Xd
  0b1_01_01010_00_1_00000_000000_11111_00000 or
;

\ `ret x30`; requires caution with SP / FP.
: asm_ret 0b110_101_1_0_0_10_11111_0000_0_0_11110_00000 ;

: asm_nop 0b110_101_01000000110010_0000_000_11111 ;

\ svc 666
: asm_svc 0b110_101_00_000_0000001010011010_000_01 ;

\ Instruction `udf 666` used for retropatching.
: ASM_PLACEHOLDER 666 ;

\ Non-immediate replacement for `literal`.
: comp_push ( C: num -- ) ( E: -- num )
  1 swap      comp_load  \ ldr x1, <num>
  asm_push_x1 comp_instr \ str x1, [x27], 8
;

: next_word ( "name" -- exec_tok ) parse_word find_word ;
: '         ( "name" -- exec_tok ) next_word  comp_push ;
: inline'   next_word inline_word ;
: postpone' next_word comp_call ;
: compile'  next_word comp_push ' comp_call comp_call ;

: drop ( val -- ) [
  27 27 8 asm_sub_imm comp_instr \ sub x27, x27, 8
] ;

\ Same as `1 pick 1 pick`.
: dup2 ( i1 i2 -- i1 i2 i1 i2 ) [
  1 2 27 -16 asm_load_pair_off comp_instr \ ldp x1, x2, [x27, -16]
      1   2  asm_push2         comp_instr \ stp x1, x2, [x27], 16
] ;

\ Same as `swap drop`.
: nip ( i1 i2 -- i2 ) [
          asm_pop_x1    comp_instr \ ldr x1, [x27, -8]!
  1 27 -8 asm_store_off comp_instr \ stur x1, [x27, -8]
] ;

\ Same as `swap2 drop2`.
: nip2 ( i1 i2 i3 i4 -- i3 i4 ) [
             asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 27 -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Same as `1 pick`.
: over ( i1 i2 -- i1 i2 i1 ) [
  1 27 -16 asm_load_off comp_instr \ ldur x1, [x27, -16]
           asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

\ Same as `3 pick 3 pick`.
: over2 ( i1 i2 i3 i4 -- i1 i2 i3 i4 i1 i2 ) [
  1 2 27 -32 asm_load_pair_off comp_instr \ ldp x1, x2, [x27, -32]
             asm_push_x1_x2    comp_instr \ stp x1, x2, [x27], 16
] ;

\ Same as `3 roll 3 roll`.
: swap2 ( i1 i2 i3 i4 -- i3 i4 i1 i2 ) [
  1 2 27 -32 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -32]
  3 4 27 -16 asm_load_pair_off  comp_instr \ ldp x3, x4, [x27, -16]
  3 4 27 -32 asm_store_pair_off comp_instr \ stp x3, x4, [x27, -32]
  1 2 27 -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Same as `-rot swap rot`.
: swap_over ( i1 i2 i3 -- i2 i1 i3 ) [
  1 2 27 -24 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -24]
  2 1 27 -24 asm_store_pair_off comp_instr \ stp x2, x1, [x27, -24]
] ;

\ Same as `over -rot`.
: dup_over ( i1 i2 -- i1 i1 i2 ) [
  1 2  27 -16 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -16]
  1 2  27 -8  asm_store_pair_off comp_instr \ stp x1, x2, [x27, -8]
    27 27  8  asm_add_imm        comp_instr \ add x27, x27, 8
] ;

\ Same as `dup rot`.
: tuck ( i1 i2 -- i2 i1 i2 ) [
  1 2 27 -16 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -16]
  2 1 27 -16 asm_store_pair_off comp_instr \ stp x2, x1, [x27, -16]
           2 asm_push1          comp_instr \ str x2, [x27], 8
] ;

: tuck2 ( i1 i2 i3 i4 -- i3 i4 i1 i2 i3 i4 ) [
  inline' swap2
  3 4 asm_push2 comp_instr \ stp x3, x4, [x27], 16
] ;

\ Same as `2 roll`.
: rot ( i1 i2 i3 -- i2 i3 i1 ) [
    1 27 -24 asm_load_off       comp_instr \ ldur x1, [x27, -24]
  2 3 27 -16 asm_load_pair_off  comp_instr \ ldp x2, x3, [x27, -16]
  2 3 27 -24 asm_store_pair_off comp_instr \ stp x2, x3, [x27, -24]
    1 27  -8 asm_store_off      comp_instr \ stur x1, [x27, -8]
] ;

: -rot ( i1 i2 i3 -- i3 i1 i2 ) [
    1 27 -24 asm_load_off       comp_instr \ ldur x1, [x27, -24]
  2 3 27 -16 asm_load_pair_off  comp_instr \ ldp x2, x3, [x27, -16]
    3 27 -24 asm_store_off      comp_instr \ stur x3, [x27, -24]
  1 2 27 -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

: rot2 ( i1 i2 i3 i4 i5 i6 -- i3 i4 i5 i6 i1 i2 ) [
  1 2 27 -48 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -48]
  3 4 27 -32 asm_load_pair_off  comp_instr \ ldp x3, x4, [x27, -32]
  5 6 27 -16 asm_load_pair_off  comp_instr \ ldp x5, x6, [x27, -16]
  3 4 27 -48 asm_store_pair_off comp_instr \ stp x3, x4, [x27, -48]
  5 6 27 -32 asm_store_pair_off comp_instr \ stp x5, x6, [x27, -32]
  1 2 27 -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Pushes the stack item found at the given index, duplicating it.
: pick ( … ind -- … [ind] ) [
           asm_pop_x1             comp_instr \ ldr x1, [x27, -8]!
  1 1      asm_mvn                comp_instr \ mvn x1, x1
  1 27 1 1 asm_load_with_register comp_instr \ ldr x1, [x27, x1, lsl 3]
           asm_push_x1            comp_instr \ str x1, [x27], 8
] ;

\ FIFO version of `pick`: starts from stack bottom.
: pick0 ( … ind -- … [ind] ) [
           asm_pop_x1             comp_instr \ ldr x1, [x27, -8]!
  1 26 1 1 asm_load_with_register comp_instr \ ldr x1, [x26, x1, lsl 3]
           asm_push_x1            comp_instr \ str x1, [x27], 8
] ;

\ Overwrite the cell at the given index with the given value.
: bury ( … val ind -- … ) [
           asm_pop_x1_x2           comp_instr \ ldp x1, x2, [x27, -16]!
  2 2      asm_mvn                 comp_instr \ mvn x2, x2
  1 27 2 1 asm_store_with_register comp_instr \ str x1, [x27, x2, lsl 3]
] ;

: flip ( i1 i2 i3 -- i3 i2 i1 ) [
  1 27 -24 asm_load_off  comp_instr \ ldr x1, [x27, -24]
  2 27 -8  asm_load_off  comp_instr \ ldr x2, [x27, -8]
  2 27 -24 asm_store_off comp_instr \ str x2, [x27, -24]
  1 27 -8  asm_store_off comp_instr \ str x1, [x27, -8]
] ;

: negate ( i1 -- i2 ) [
    asm_pop_x1  comp_instr \ ldr x1, [x27, -8]!
  1 asm_neg     comp_instr \ neg x1, x1
    asm_push_x1 comp_instr \ str x1, [x27], 8
] ;

: + ( i1 i2 -- i3 ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 1 2 asm_add_reg   comp_instr \ add x1, x1, x2
        asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: - ( i1 i2 -- i3 ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 1 2 asm_sub_reg   comp_instr \ sub x1, x1, x2
        asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: * ( i1 i2 -- i3 ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 1 2 asm_mul       comp_instr \ mul x1, x1, x2
        asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: / ( i1 i2 -- i3 ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 1 2 asm_sdiv      comp_instr \ sdiv x1, x1, x2
        asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: mod ( i1 i2 -- i3 ) [
          asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
    3 1 2 asm_sdiv      comp_instr \ sdiv x3, x1, x2
  1 3 2 1 asm_msub      comp_instr \ msub x1, x3, x2, x1
          asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: /mod ( dividend divisor -- rem quo ) [
      1 2 asm_pop2  comp_instr \ ldp x1, x2, [x27, -16]!
    3 1 2 asm_sdiv  comp_instr \ sdiv x3, x1, x2
  1 3 2 1 asm_msub  comp_instr \ msub x1, x3, x2, x1
      1 3 asm_push2 comp_instr \ stp x1, x3, [x27], 16
] ;

: abs ( ±int -- +int ) [
               asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
             1 asm_cmp_zero comp_instr \ cmp x1, 0
  1 1 1 ASM_PL asm_csneg    comp_instr \ cneg x1, x1, mi
               asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: min ( i1 i2 -- i1|i2 ) [
               asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
           1 2 asm_cmp_reg   comp_instr \ cmp x1, x2
  1 1 2 ASM_LT asm_csel      comp_instr \ csel x1, x1, x2, lt
               asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: max ( i1 i2 -- i1|i2 ) [
               asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
           1 2 asm_cmp_reg   comp_instr \ cmp x1, x2
  1 1 2 ASM_GT asm_csel      comp_instr \ csel x1, x1, x2, gt
               asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: asm_comp_cset_cond ( cond -- )
         asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
     1 2 asm_cmp_reg   comp_instr \ cmp x1, x2
  1 swap asm_cset      comp_instr \ cset x1, <cond>
         asm_push_x1   comp_instr \ str x1, [x27], 8
;

: asm_comp_cset_zero ( cond -- )
         asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
       1 asm_cmp_zero comp_instr \ cmp x1, 0
  1 swap asm_cset     comp_instr \ cset x1, <cond>
         asm_push_x1  comp_instr \ str x1, [x27], 8
;

\ https://gforth.org/manual/Numeric-comparison.html
: =   ( i1 i2 -- bool ) [ ASM_EQ asm_comp_cset_cond ] ;
: <>  ( i1 i2 -- bool ) [ ASM_NE asm_comp_cset_cond ] ;
: >   ( i1 i2 -- bool ) [ ASM_GT asm_comp_cset_cond ] ;
: <   ( i1 i2 -- bool ) [ ASM_LT asm_comp_cset_cond ] ;
: <=  ( i1 i2 -- bool ) [ ASM_LE asm_comp_cset_cond ] ;
: >=  ( i1 i2 -- bool ) [ ASM_GE asm_comp_cset_cond ] ;
: <0  ( num   -- bool ) [ ASM_LT asm_comp_cset_zero ] ; \ Or `MI`.
: >0  ( num   -- bool ) [ ASM_GT asm_comp_cset_zero ] ;
: <=0 ( num   -- bool ) [ ASM_LE asm_comp_cset_zero ] ;
: >=0 ( num   -- bool ) [ ASM_GE asm_comp_cset_zero ] ; \ Or `PL`.

: odd ( i1 -- bool ) 1 and ;

: @ ( adr -- val ) [
        asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
  1 1 0 asm_load_off comp_instr \ ldur x1, [x1]
        asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: ! ( val adr -- ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_off comp_instr \ str x1, [x2]
] ;

\ 32-bit version of `!`. Used for patching instructions.
: !32 ( val adr -- ) [
        asm_pop_x1_x2    comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_off_32 comp_instr \ str w1, x2
] ;

: on!  ( adr -- ) 1 swap ! ;
: off! ( adr -- ) 0 swap ! ;

\ Floor of data stack.
: sp0 ( -- adr ) [ 26 asm_push1 comp_instr ] ; \ str x26, [x27], 8

(
Pushes the address of the next writable stack cell.
Like `sp@` in Gforth but renamed to make sense.
Note: our integer stack is empty-ascending.

Should have been `str x27, [x27], 8`, but Arm64 has a quirk where an
address-modifying load or store which also uses the address register
in the value position is considered to be "unpredictable". Different
CPUs are allowed to handle this differently; on Apple Silicon chips,
this is considered a "bad instruction" and blows up.
)
: sp ( -- adr ) [
  27 27 0 asm_store_off comp_instr \ stur x27, [x27]
  27 27 8 asm_add_imm   comp_instr \ add x27, x27, 8
] ;

\ Sets the stack pointer register to the given address.
\ Uses two instructions for the same reason as the above.
: sp! ( adr -- ) [
       asm_pop_x1  comp_instr \ ldr x1, [x27, -8]!
  27 1 asm_mov_reg comp_instr \ mov x27, x1
] ;

: cell 8 ;
: cells ( len -- size ) 3 lsl ;
: /cells ( size -- len ) 3 asr ;

\ Stack introspection doesn't need to be optimal.
: depth ( -- len ) sp sp0 - /cells ;
: stack_clear ( … -- ) sp0 sp! ;
: stack_trunc ( … len -- … ) cells sp0 + sp! ;

: c@ ( str -- char ) [
        asm_pop_x1        comp_instr \ ldr x1, [x27, -8]!
  1 1 0 asm_load_byte_off comp_instr \ ldrb x1, [x1]
        asm_push_x1       comp_instr \ str x1, [x27], 8
] ;

: c! ( char adr -- ) [
        asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_byte_off comp_instr \ strb x1, [x2]
] ;

: char' parse_word drop c@ comp_push ;

\ Core primitive for locals.
: to: ( C: "name" -- ) ( E: val -- ) parse_word comp_local_ind comp_local_pop ;

: nop ;
: pc_off ( adr -- adr off ) here over - ; \ For forward jumps.
: reserve_instr ASM_PLACEHOLDER comp_instr ;
: reserve_cond asm_pop_x1 comp_instr here reserve_instr ;

(
Conditionals work like this:

  #if        \ cbz <outside>
    if_true
  #end
  outside

  #if        \ cbz <if_false>
    if_true
  #else      \ b <outside>
    if_false
  #end
  outside

Unlike other Forth systems, we also support chains
of "elif" terminated with a single "end".
)

\ b <pc_off>
: patch_uncond_forward ( adr -- ) pc_off asm_branch swap !32 ;

\ b -<pc_off>
: patch_uncond_back ( adr -- ) here - asm_branch swap !32 ;

\ cbz x1, <else|end>
: if_patch ( adr[if] -- ) pc_off 1 swap asm_cmp_branch_zero swap !32 ;

\ cbnz x1, <else|end>
: ifn_patch ( adr[ifn] -- ) pc_off 1 swap asm_cmp_branch_not_zero swap !32 ;

: if_pop ( fun[prev] adr[if]  -- ) if_patch execute ;
: ifn_pop ( fun[prev] adr[ifn]  -- ) ifn_patch execute ;

\ This strange setup allows `#end` to chain-call `#else`,
\ and terminate the chain at the topmost `#if` or `#ifn`.
: cond_init ( -- fun[nop] fun[exec] adr[cond] )
  ' nop ' execute reserve_cond
;

: #if ( C: -- fun[nop] fun[exec] adr[if] fun[if] ) ( E: pred -- )
  cond_init ' if_pop
;

: #ifn ( C: -- fun[nop] fun[exec] adr[ifn] fun[ifn] ) ( E: pred -- )
  cond_init ' ifn_pop
;

\ Unlike "if", "elif" doesn't terminate the call chain with a nop.
: #elif ( C: -- fun[exec] adr[elif] fun[elif] ) ( E: pred -- )
  ' execute reserve_cond ' if_pop
;

: #elifn ( C: -- fun[exec] adr[elif] fun[elif] ) ( E: pred -- )
  ' execute reserve_cond ' ifn_pop
;

: else_pop ( fun[prev] adr[else] -- ) patch_uncond_forward execute ;

: #else
  ( C: fun[exec] adr[if] fun[if] -- adr[else] fun[else] )
  here to: adr reserve_instr

  \ Turn `fun[exec]` into `fun[nop]` to prevent "if" from chaining.
  ' nop 2 bury

  execute
  adr ' else_pop
;

: #end execute ;

: #begin ( C: -- adr[beg] ) here ;
: #again ( C: adr[beg] -- ) here - asm_branch comp_instr ; \ b <begin>

(
Currently unusable because we don't support mixing loop-related words
with conditionals. Both groups use the regular stack for control info.
Inside a conditional, `#leave` gets unjustly terminated by the nearest
`#else` or `#end`. Also, loop words keep `adr[beg]` at the top.
)
: #leave ( C: adr[beg] -- adr[b] fun[b] adr[beg] )
  here ' patch_uncond_forward reserve_instr rot
;

(
The topmost `#while` is terminated with `#repeat`.
Each additional `#while` can be terminated with `#end`.

  #begin
    cond #while
    cond #while
    cond #while
  #repeat #end #end
)
: #while ( C: adr[beg] -- adr[cbz] fun[cbz] fun[beg] ) ( E: pred -- )
  reserve_cond ' if_patch rot
;

\ For use with `#while`; potentially more general.
: #repeat ( C: adr[any] fun[any] adr[beg] -- )
  postpone' #again execute
;

: #until ( C: adr[beg] -- ) ( E: pred -- )
  asm_pop_x1                            comp_instr \ Load predicate.
  here - 1 swap asm_cmp_branch_not_zero comp_instr \ cbz 1, <begin>
;

(
Similar to the standard `?do … loop`, with clearer naming.
Keeps the provided ceiling and floor on the regular stack.
Does NOT discard them when done. There's no magic `i`.

  8 0 #do> dup . #loop+ drop2
  \ 0 1 2 3 4 5 6 7
)
: #do> ( C: -- adr[cbz] fun[cbz] adr[beg] ) ( E: ceil floor -- ceil floor )
  here compile' dup2 compile' > postpone' #while
;

: #loop+ ( C: fun[any] fun[any] adr[beg] -- )
  compile' inc postpone' #repeat
;

: ?dup ( val bool -- val ?val ) #if dup #end ;

\ Same as standard `s"` in interpretation mode.
\ The parser ensures a trailing null byte.
\ All our strings are zero-terminated.
: parse_str ( -- cstr len ) char' " parse ;
: parse_cstr ( -- cstr ) parse_str drop ;

\ Same as standard `s"` in interpretation mode.
\ For top-level code in scripts.
\ Inside words, use `"`.
: str" ( -- cstr len ) parse_str ;

: comp_str ( C: <str> -- ) ( E: -- cstr len )
  parse_str tuck            ( len str len )
  inc                       \ Reserve 1 more for null byte.
  1              comp_const \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  2 swap         comp_load  \ ldr x2, <len>
  asm_push_x1_x2 comp_instr \ stp x1, x2, [x27], 16
;

\ Same as standard `s"` in compilation mode.
\ Words ending with a quote are automatically immediate.
: " comp_str ;

\ For top-level code in scripts.
: cstr" ( C: <str> -- ) ( E: -- cstr ) parse_str drop ;

: comp_cstr ( C: <str> -- ) ( E: -- cstr )
  parse_str   inc        \ Reserve 1 more for null byte.
  1           comp_const \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr \ stp x1, x2, [x27], 16
;

\ For use inside words.
\ Words ending with a quote are automatically immediate.
: c" comp_cstr ;

(
The program is implicitly linked with `libc`.
We're free to use Posix-standard C functions.
C variables like `stdout` are a bit trickier,
but also optional; we could `fdopen` instead.
)

2 1 extern: calloc  ( len size -- addr )
1 1 extern: malloc  ( size -- addr )
1 0 extern: free    ( addr -- )
3 0 extern: memset  ( buf char len -- )
1 1 extern: strlen  ( cstr -- len )
3 1 extern: strncmp ( str0 str1 len -- )
3 0 extern: strncpy ( buf[tar] buf[src] len -- )
3 0 extern: strlcpy ( buf[tar] buf[src] buf_len -- )

: move    ( buf[src] buf[tar] len -- ) swap_over strncpy ;
: fill    ( buf len char -- ) swap memset ;
: erase   ( buf len -- ) 0 fill ;
: blank   ( buf len -- ) 32 fill ;
: compare ( str0 len0 str1 len1 -- direction ) rot min strncmp ;
: str=    ( str0 len0 str1 len1 -- bool ) compare =0 ;
: str<    ( str0 len0 str1 len1 -- bool ) compare <0 ;
: str<>   ( str0 len0 str1 len1 -- bool ) compare <>0 ;

extern_ptr: __stdinp
extern_ptr: __stdoutp
extern_ptr: __stderrp

: stdin  __stdinp  @ ;
: stdout __stdoutp @ ;
: stderr __stderrp @ ;

4 0 extern: fwrite
1 0 extern: putchar
2 0 extern: fputc
2 0 extern: fputs
1 0 extern: printf
2 0 extern: fprintf
3 0 extern: snprintf
1 0 extern: fflush

: eputchar ( code -- ) stderr fputc ;
: puts ( cstr -- ) stdout fputs ;
: eputs ( cstr -- ) stderr fputs ;
: flush stdout fflush ;
: eflush stderr fflush ;

: type ( str len -- ) 1 swap stdout fwrite ;
: etype ( str len -- ) 1 swap stderr fwrite ;
: cr 10 putchar ;
: ecr 10 putchar ;
: space 32 putchar ;

: log" comp_cstr compile' puts ;
: elog" comp_cstr compile' eputs ;

: } ( … len -- )
  #begin
    dup >0 #while
    swap comp_local_pop
    dec
  #repeat
  drop
;

(
Support for the `{ inp0 inp1 -- out0 out1 }` locals notation.
Capture order matches stack push order and the parens notation,
like in Gforth and VfxForth. Types are not supported.
)
: {
  [ immediate ]
  0 to: loc_len

  #begin
    parse_word to: len to: str
    " }" str len str=
    #if loc_len } #ret #end

    " --" str len str<> #while \ `--` to `}` is a comment.

    str len comp_local_ind \ Pushes new local index.
    loc_len inc to: loc_len
  #repeat

  \ After `--`: skip all words which aren't `}`.
  #begin parse_word " }" str<> #until

  loc_len }
;

\ For compiling words which modify a local by applying the given function.
: mut_local ( C: fun "name" -- ) ( E: -- )
  parse_word
  comp_local_ind dup
  comp_local_push
  swap comp_call
  comp_local_pop
;

\ For compiling words which modify a local by applying the given function.
: mut_local' ( C: "name" fun -- ) ( E: -- )
  postpone' '
  compile' mut_local
;

\ Usage: `++: some_local`.
: ++: ( C: "name" -- ) ( E: -- ) mut_local' inc ;
: --: ( C: "name" -- ) ( E: -- ) mut_local' dec ;

: within { num one two -- bool }
  one num >
  num two <=
  =0 or =0
;

: align_down ( size width -- size ) negate and ;
: align_up   { size width -- size } width dec size + width align_down ;

(
On Arm64, we can't modify `sp` by 8 bytes and leave it like that.
The ABI requires it to be aligned to 16 bytes when loading or storing.
We store a pair to have a predictable value in the upper 8 bytes,
instead of leaving random stack garbage there.

TODO better name: this pops one stack item, but `dup` implies otherwise.
)
: dup>systack [
  inline
             asm_pop_x1         comp_instr \ ldr x1, [x27, -8]!
  1 1 31 -16 asm_store_pair_pre comp_instr \ stp x1, x1, [sp, -16]!
] ;

: pair>systack ( Forth: i1 i2 -- | System: -- i1 i2 ) [
  inline
             asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 31 -16 asm_store_pair_pre comp_instr \ stp x1, x2, [sp, -16]!
] ;

(
Note: `add` is one of the few instructions which treat `x31` as `sp`
rather than `xzr`. `add x1, sp, 0` disassembles as `mov x1, sp`.
)
: systack_ptr ( -- sp ) [
  inline
  1 31 0 asm_add_imm comp_instr \ add x1, sp, 0
         asm_push_x1 comp_instr \ str x1, [x27], 8
] ;

: asm_comp_systack_push ( len -- )
  dup <=0 #if drop #ret #end

  dup odd #if
    ' dup>systack inline_word
    dec
  #end

  dup #ifn drop #ret #end

  #begin
    dup #while
    ' pair>systack inline_word
    2 -
  #repeat
  drop
;

: asm_comp_systack_pop ( len -- )
  dup <=0 #if #ret #end
  cells 16 align_up
  31 31 rot asm_add_imm comp_instr \ add sp, sp, <size>
;

(
Short for "varargs". Sets up arguments for a variadic call.
Assumes the Apple Arm64 ABI where varargs use the systack.
Usage:

  : some_word
    #c" numbers: %zd %zd %zd"
    10 20 30 [ 3 va- ] printf [ -va ] cr
  ;

Caution: varargs can only be used in direct calls to variadic procedures.
Indirect calls DO NOT WORK because the stack pointer is changed by calls.
)
: va- ( C: len -- len ) ( E: <systack_push> ) dup asm_comp_systack_push ;
: -va ( C: len -- )     ( E: <systack_pop> )      asm_comp_systack_pop ;

(
Format-prints to stdout using `printf`. `N` is the variadic arg count,
which must be available at compile time. Usage example:

  10 20 30 [ 3 ] logf" numbers: %zu %zu %zu" cr
)
: logf" ( C: N -- ) ( E: i1 … iN -- )
  va- postpone' c" compile' printf -va
;

\ Format-prints to stderr.
: elogf" ( C: N -- ) ( E: i1 … iN -- )
  va- compile' stderr postpone' c" compile' fprintf -va
;

(
Formats into the provided buffer using `snprintf`. Usage example:

  SOME_BUF 10 20 30 [ 3 ] sf" numbers: %zu %zu %zu" cr
)
: sf" ( C: N -- ) ( E: buf size i1 … iN -- )
  va- comp_cstr compile' snprintf -va
;

(
`x0` is our error register. The annotation `throws` makes the compiler
insert an error check after each call to this procedure and any other
procedure which calls this one, turning the error into an exception.
The `len` parameter is here because buffer variables return it.
)
: throw ( cstr len -- )
  [ throws ]
  drop
  [ 0 asm_pop1 comp_instr ] \ ldr x0, [x27, -8]!
;

(
Usage: `throw" some_error_msg"`.

Also see `sthrowf"` for formatted errors.
)
: throw" comp_str compile' throw ;

(
Formats an error message into the provided buffer using `snprintf`,
then throws the buffer as the error value. The buffer must be zero
terminated; `buf:` ensures this automatically. Usage example:

  4096 buf: SOME_BUF
  SOME_BUF 10 20 30 [ 20 ] sthrowf" error codes: %zu %zu %zu"

Also see `throwf"` which comes with its own buffer.
)
: sthrowf" ( C: len -- ) ( E: buf size i1 … iN -- )
  va-
  compile'  dup2
  comp_cstr
  compile'  snprintf
  -va
  compile'  throw
;

: log_int ( num -- ) [ 1 ] logf" %zd"  ;
: log_cell ( num ind -- ) dup_over [ 3 ] logf" %zd 0x%zx <%zd>" ;
: . ( num -- ) depth dec log_cell cr ;

: .s
  depth to: len

  len #ifn
    log" stack is empty" cr
    #ret
  #end

  len <0 #if
    log" stack length is negative: " len log_int cr
    #ret
  #end

  len [ 1 ] logf" stack <%zd>:" cr

  0
  #begin
    dup to: ind
    ind len < #while
    space space
    ind pick0 ind log_cell cr
    inc
  #repeat
  drop
;

: .sc .s stack_clear ;

\ For words which define words. Kinda like `create`.
: #word_beg compile' : ;
: #word_end compile' ; not_comp_only ;

(
Words for declaring constants and variables. Unlike in standard Forth,
variables also take an initial value.
)

\ Same as standard `constant`.
: let: ( C: val -- ) ( E: -- val ) #word_beg comp_push inline #word_end ;

\ Similar to standard `variable`.
: var: ( C: init "name" -- ) ( E: -- addr )
  #word_beg
  cell 1      comp_static \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr  \ str x1, [x27], 8
  #word_end
  !
;

(
Creates a global variable which refers to a buffer of at least
the given size in bytes, rounded up to page size.

Serves the same role as the standard idiom `create <name> N allot`
which we don't have because we segregate code and data.
)
: buf: ( C: size "name" -- ) ( E: -- addr size )
  #word_beg
  dup 1       comp_static drop \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr       \ str x1, [x27], 8
              comp_push        \ str <len>, [x27], 8
  #word_end
;

0 let: false
1 let: true

4096 buf: ERR_BUF

(
Like `sthrowf"` but easier to use. Example:

  10 20 30 [ 20 ] throwf" error codes: %zu %zu %zu"
)
: throwf" ( C: len -- ) ( E: i1 … iN -- )
  va-
  compile'  ERR_BUF
  comp_cstr
  compile'  snprintf
  -va
  compile' ERR_BUF
  compile' throw
;

1 1 extern: strerror
extern_ptr: __error
: errno __error @ ;

6 1 extern: mmap
3 1 extern: mprotect

16384 let: PAGE_SIZE
0     let: PROT_NONE
1     let: PROT_READ
2     let: PROT_WRITE
2     let: MAP_PRIVATE
4096  let: MAP_ANON

: mem_map { size pflag -- addr }
  MAP_ANON MAP_PRIVATE or to: mflag
  0 size pflag mflag -1 0 mmap
  dup -1 = #if drop throw" unable to mmap" #end
;

: mem_unprot ( addr size -- )
  PROT_READ PROT_WRITE or mprotect
  -1 = #if throw" unable to mprotect" #end
;

(
Allocates a guarded buffer: `guard|data|guard`.
Attempting to read or write inside the guards,
aka underflow or overflow, triggers a segfault.
)
: mem_alloc ( size1 -- addr size2 )
        PAGE_SIZE align_up to: size2
  size2 PAGE_SIZE 1 lsl +  to: size
  size  PROT_NONE mem_map  to: addr
  addr  PAGE_SIZE +        to: addr
  addr  size2 mem_unprot
  addr  size2
;
