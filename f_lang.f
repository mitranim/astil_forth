: drop2 [ 0b1_1_0_100010_0_000000010000_11011_11011 comp_instr ] ;
:: \ 10 parse drop2 ;
:: ( 41 parse drop2 ;

\ Can use comments now!
\
\ This file boostraps the Forth language. The outer interpreter / compiler
\ provides only the most fundamental intrinsics; enough for self-assembly.
\ We define basic words in terms of machine instructions, building up from
\ there. Currently only the Arm64 CPU architecture is supported.

\ brk 666
: abort [ 0b110_101_00_001_0000001010011010_000_00 comp_instr ] ;
: unreachable abort ;
: nop ;

: ASM_INSTR_SIZE  4      ;
: ARCH_REG_DAT_SP  27     ;
: ARCH_REG_FP      29     ;
: ARCH_REG_SP      31     ;
: ASM_EQ          0b0000 ;
: ASM_NE          0b0001 ;
: ASM_CS          0b0010 ;
: ASM_CC          0b0011 ;
: ASM_MI          0b0100 ;
: ASM_PL          0b0101 ;
: ASM_VS          0b0110 ;
: ASM_VC          0b0111 ;
: ASM_HI          0b1000 ;
: ASM_LS          0b1001 ;
: ASM_GE          0b1010 ;
: ASM_LT          0b1011 ;
: ASM_GT          0b1100 ;
: ASM_LE          0b1101 ;
: ASM_AL          0b1110 ;
: ASM_NV          0b1111 ;
: ASM_PLACEHOLDER 666    ; \ udf 666; used in retropatching.

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
: asm_pattern_load_store_pair ( Xt1 Xt2 Xn imm7 -- instr )
  3 lsr 7 bit_trunc 15 lsl    \ imm7
  swap              5  lsl or \ Xn
  swap              10 lsl or \ Xt2
                           or \ Xt1
;

\ ldp Xt1, Xt2, [Xn, <imm>]!
: asm_load_pair_pre ( Xt1 Xt2 Xn imm7 -- instr )
  asm_pattern_load_store_pair
  0b10_101_0_011_1_0000000_00000_00000_00000 or
;

\ ldp Xt1, Xt2, [Xn], <imm>
: asm_load_pair_post ( Xt1 Xt2 Xn imm7 -- instr )
  asm_pattern_load_store_pair
  0b10_101_0_001_1_0000000_00000_00000_00000 or
;

\ ldp Xt1, Xt2, [Xn, <imm>]
: asm_load_pair_off ( Xt1 Xt2 Xn imm7 -- instr )
  asm_pattern_load_store_pair
  0b10_101_0_010_1_0000000_00000_00000_00000 or
;

\ stp Xt1, Xt2, [Xn, <imm>]!
: asm_store_pair_pre ( Xt1 Xt2 Xn imm7 -- instr )
  asm_pattern_load_store_pair
  0b10_101_0_011_0_0000000_00000_00000_00000 or
;

\ stp Xt1, Xt2, [Xn], <imm>
: asm_store_pair_post ( Xt1 Xt2 Xn imm7 -- instr )
  asm_pattern_load_store_pair
  0b10_101_0_001_0_0000000_00000_00000_00000 or
;

\ stp Xt1, Xt2, [Xn, <imm>]
: asm_store_pair_off ( Xt1 Xt2 Xn imm7 -- instr )
  asm_pattern_load_store_pair
  0b10_101_0_010_0_0000000_00000_00000_00000 or
;

\ Shared by a lot of load and store instructions.
\ Patches these bits:
\   0b00_000_0_00_00_0_XXXXXXXXX_00_XXXXX_XXXXX
: asm_pattern_load_store ( Xt Xn imm9 -- instr )
  9 bit_trunc 12 lsl    \ imm9
  swap        5  lsl or \ Xn
                     or \ Xt
;

\ ldr Xt, [Xn, <imm>]!
: asm_load_pre ( Xt Xn imm9 -- instr )
  asm_pattern_load_store
  0b11_111_0_00_01_0_000000000_11_00000_00000 or
;

\ ldr Xt, [Xn], <imm>
: asm_load_post ( Xt Xn imm9 -- instr )
  asm_pattern_load_store
  0b11_111_0_00_01_0_000000000_01_00000_00000 or
;

\ str Xt, [Xn, <imm>]!
: asm_store_pre ( Xt Xn imm9 -- instr )
  asm_pattern_load_store
  0b11_111_0_00_00_0_000000000_11_00000_00000 or
;

\ str Xt, [Xn], <imm>
: asm_store_post ( Xt Xn imm9 -- instr )
  asm_pattern_load_store
  0b11_111_0_00_00_0_000000000_01_00000_00000 or
;

\ ldur Xt, [Xn, <imm>]
: asm_load_off ( Xt Xn imm9 -- instr )
  asm_pattern_load_store
  0b11_111_0_00_01_0_000000000_00_00000_00000 or
;

\ stur Xt, [Xn, <imm>]
: asm_store_off ( Xt Xn imm9 -- instr )
  asm_pattern_load_store
  0b11_111_0_00_00_0_000000000_00_00000_00000 or
;

\ ldur Wt, [Xn, <imm>]
: asm_load_off_32 ( Wt Xn imm9 -- instr )
  asm_pattern_load_store
  0b10_111_0_00_01_0_000000000_00_00000_00000 or
;

\ stur Wt, [Xn, <imm>]
: asm_store_off_32 ( Wt Xn imm9 -- instr )
  asm_pattern_load_store
  0b10_111_0_00_00_0_000000000_00_00000_00000 or
;

\ Shared by register-offset load and store instructions.
\ `scale` must be 0 or 1; if 1, offset is multiplied by 8.
\ Assumes `lsl` is already part of the base instruction.
: asm_pattern_load_store_with_register ( Xt Xn Xm scale -- instr )
  <>0  12 lsl    \ lsl 3
  swap 16 lsl or \ Xm
  swap 5  lsl or \ Xn
              or \ Xt
;

\ ldr Xt, [Xn, Xm, lsl <scale>]
: asm_load_with_register ( Xt Xn Xm scale -- instr )
  asm_pattern_load_store_with_register
  0b11_111_0_00_01_1_00000_011_0_10_00000_00000 or
;

\ str Xt, [Xn, Xm, lsl <scale>]
: asm_store_with_register ( Xt Xn Xm scale -- instr )
  asm_pattern_load_store_with_register
  0b11_111_0_00_00_1_00000_011_0_10_00000_00000 or
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

\ Immediate offset must be unsigned.
\ ldr Xt, [Xn, <imm>]
: asm_load_scaled_off ( Xt Xn imm9 -- instr )
  3 lsr asm_pattern_arith_imm
  0b11_111_0_01_01_000000000000_00000_00000 or
;

\ Immediate offset must be unsigned.
\ str Xt, [Xn, <imm>]
: asm_store_scaled_off ( Xt Xn imm9 -- instr )
  3 lsr asm_pattern_arith_imm
  0b11_111_0_01_00_000000000000_00000_00000 or
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
: asm_push1 ( Xd -- instr ) ARCH_REG_DAT_SP 8 asm_store_post ;

\ ldr Xd, [x27, -8]!
: asm_pop1 ( Xd -- instr ) ARCH_REG_DAT_SP -8 asm_load_pre ;

\ stp Xt1, Xt2, [x27], 16
: asm_push2 ( Xt1 Xt2 -- instr ) ARCH_REG_DAT_SP 16 asm_store_pair_post ;

\ ldp Xt1, Xt2, [x27, -16]!
: asm_pop2 ( Xt1 Xt2 -- instr ) ARCH_REG_DAT_SP -16 asm_load_pair_pre ;

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

\ sub Xd, Xn, Xm, lsl 3
: asm_sub_reg_words ( Xd Xn Xm -- instr )
  asm_sub_reg
  0b0_0_0_00000_00_0_00000_000011_00000_00000 or
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
: asm_pattern_compare_branch ( Xt imm19 -- instr )
  2 lsr 19 bit_trunc 5 lsl \ imm19
                        or \ Xt
;

\ cbz x1, <off>
: asm_cmp_branch_zero ( Xt imm19 -- instr )
  asm_pattern_compare_branch
  0b1_011010_0_0000000000000000000_00000 or
;

\ cbnz x1, <off>
: asm_cmp_branch_not_zero ( Xt imm19 -- instr )
  asm_pattern_compare_branch
  0b1_011010_1_0000000000000000000_00000 or
;

\ Same as `0 pick`.
: dup ( i1 -- i1 i1 ) [
  1 ARCH_REG_DAT_SP -8 asm_load_off comp_instr \ ldur x1, [x27, -8]
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

\ Non-immediate replacement for `literal`.
: comp_push ( C: num -- ) ( E: -- num )
  1           comp_load  \ ldr x1, <num>
  asm_push_x1 comp_instr \ str x1, [x27], 8
;

\ SYNC[wordlist_enum].
: WORDLIST_EXEC 1 ;
: WORDLIST_COMP 2 ;

: next_word ( wordlist "word" -- exec_tok ) parse_word find_word ;

: tick_next ( C: wordlist "word" -- ) ( E: -- exec_tok ) next_word comp_push ;
:: '  WORDLIST_EXEC tick_next ;
:: '' WORDLIST_COMP tick_next ;

:  inline_next ( wordlist "word" -- ) ( E: <word> ) next_word inline_word ;
:: inline'  WORDLIST_EXEC inline_next ;
:: inline'' WORDLIST_COMP inline_next ;

\ Note: `execute` is renamed from `postpone`.
: execute_next ( wordlist "word" -- ) ( E: <word> ) next_word comp_call ;
:: execute'  WORDLIST_EXEC execute_next ;
:: execute'' WORDLIST_COMP execute_next ;

:  compile_next ( wordlist "word" -- ) next_word comp_push ' comp_call comp_call ;
:: compile'  WORDLIST_EXEC compile_next ;
:: compile'' WORDLIST_COMP compile_next ;

: drop ( val -- ) [
  ARCH_REG_DAT_SP ARCH_REG_DAT_SP 8 asm_sub_imm comp_instr \ sub x27, x27, 8
] ;

\ Same as `1 pick 1 pick`.
: dup2 ( i1 i2 -- i1 i2 i1 i2 ) [
  1 2 ARCH_REG_DAT_SP -16 asm_load_pair_off comp_instr \ ldp x1, x2, [x27, -16]
  1 2                     asm_push2         comp_instr \ stp x1, x2, [x27], 16
] ;

\ Same as `swap drop`.
: nip ( i1 i2 -- i2 ) [
                       asm_pop_x1    comp_instr \ ldr x1, [x27, -8]!
  1 ARCH_REG_DAT_SP -8 asm_store_off comp_instr \ stur x1, [x27, -8]
] ;

\ Same as `swap2 drop2`.
: nip2 ( i1 i2 i3 i4 -- i3 i4 ) [
                          asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 ARCH_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Same as `1 pick`.
: over ( i1 i2 -- i1 i2 i1 ) [
  1 ARCH_REG_DAT_SP -16 asm_load_off comp_instr \ ldur x1, [x27, -16]
                        asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

\ Same as `3 pick 3 pick`.
: over2 ( i1 i2 i3 i4 -- i1 i2 i3 i4 i1 i2 ) [
  1 2 ARCH_REG_DAT_SP -32 asm_load_pair_off comp_instr \ ldp x1, x2, [x27, -32]
                          asm_push_x1_x2    comp_instr \ stp x1, x2, [x27], 16
] ;

\ Same as `3 roll 3 roll`.
: swap2 ( i1 i2 i3 i4 -- i3 i4 i1 i2 ) [
  1 2 ARCH_REG_DAT_SP -32 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -32]
  3 4 ARCH_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x3, x4, [x27, -16]
  3 4 ARCH_REG_DAT_SP -32 asm_store_pair_off comp_instr \ stp x3, x4, [x27, -32]
  1 2 ARCH_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Same as `-rot swap rot`.
: swap_over ( i1 i2 i3 -- i2 i1 i3 ) [
  1 2 ARCH_REG_DAT_SP -24 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -24]
  2 1 ARCH_REG_DAT_SP -24 asm_store_pair_off comp_instr \ stp x2, x1, [x27, -24]
] ;

\ Same as `over -rot`.
: dup_over ( i1 i2 -- i1 i1 i2 ) [
  1 2             ARCH_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -16]
  1 2             ARCH_REG_DAT_SP -8  asm_store_pair_off comp_instr \ stp x1, x2, [x27, -8]
  ARCH_REG_DAT_SP ARCH_REG_DAT_SP  8  asm_add_imm        comp_instr \ add x27, x27, 8
] ;

\ Same as `dup rot`.
: tuck ( i1 i2 -- i2 i1 i2 ) [
  1 2 ARCH_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -16]
  2 1 ARCH_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x2, x1, [x27, -16]
                       2  asm_push1          comp_instr \ str x2, [x27], 8
] ;

: tuck2 ( i1 i2 i3 i4 -- i3 i4 i1 i2 i3 i4 ) [
  inline' swap2
  3 4 asm_push2 comp_instr \ stp x3, x4, [x27], 16
] ;

\ Same as `2 roll`.
: rot ( i1 i2 i3 -- i2 i3 i1 ) [
    1 ARCH_REG_DAT_SP -24 asm_load_off       comp_instr \ ldur x1, [x27, -24]
  2 3 ARCH_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x2, x3, [x27, -16]
  2 3 ARCH_REG_DAT_SP -24 asm_store_pair_off comp_instr \ stp x2, x3, [x27, -24]
    1 ARCH_REG_DAT_SP  -8 asm_store_off      comp_instr \ stur x1, [x27, -8]
] ;

: -rot ( i1 i2 i3 -- i3 i1 i2 ) [
    1 ARCH_REG_DAT_SP -24 asm_load_off       comp_instr \ ldur x1, [x27, -24]
  2 3 ARCH_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x2, x3, [x27, -16]
    3 ARCH_REG_DAT_SP -24 asm_store_off      comp_instr \ stur x3, [x27, -24]
  1 2 ARCH_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

: rot2 ( i1 i2 i3 i4 i5 i6 -- i3 i4 i5 i6 i1 i2 ) [
  1 2 ARCH_REG_DAT_SP -48 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -48]
  3 4 ARCH_REG_DAT_SP -32 asm_load_pair_off  comp_instr \ ldp x3, x4, [x27, -32]
  5 6 ARCH_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x5, x6, [x27, -16]
  3 4 ARCH_REG_DAT_SP -48 asm_store_pair_off comp_instr \ stp x3, x4, [x27, -48]
  5 6 ARCH_REG_DAT_SP -32 asm_store_pair_off comp_instr \ stp x5, x6, [x27, -32]
  1 2 ARCH_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Pushes the stack item found at the given index, duplicating it.
: pick ( … ind -- … [ind] ) [
                        asm_pop_x1             comp_instr \ ldr x1, [x27, -8]!
  1 1                   asm_mvn                comp_instr \ mvn x1, x1
  1 ARCH_REG_DAT_SP 1 1 asm_load_with_register comp_instr \ ldr x1, [x27, x1, lsl 3]
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
  2 2                   asm_mvn                 comp_instr \ mvn x2, x2
  1 ARCH_REG_DAT_SP 2 1 asm_store_with_register comp_instr \ str x1, [x27, x2, lsl 3]
] ;

: flip ( i1 i2 i3 -- i3 i2 i1 ) [
  1 ARCH_REG_DAT_SP -24 asm_load_off  comp_instr \ ldr x1, [x27, -24]
  2 ARCH_REG_DAT_SP -8  asm_load_off  comp_instr \ ldr x2, [x27, -8]
  2 ARCH_REG_DAT_SP -24 asm_store_off comp_instr \ str x2, [x27, -24]
  1 ARCH_REG_DAT_SP -8  asm_store_off comp_instr \ str x1, [x27, -8]
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

\ lsl Xd, Xn, <imm>
: asm_lsl_imm ( Xd Xn imm6 -- instr )
  dup negate 64 mod 6 bit_trunc 16 lsl    \ immr
  63 rot -          6 bit_trunc 10 lsl or \ imms
  swap                           5 lsl or \ Xn
                                       or \ Xd
  0b1_10_100110_1_000000_000000_00000_00000 or
;

\ tbz Xt, <bit>, <imm>
: asm_test_bit_branch_zero ( Xt bit imm14 -- instr )
  2 lsr 14 bit_trunc 5 lsl    \ imm14
  over        5 lsr 31 lsl or \ b5
  swap  5 bit_trunc 19 lsl or \ b40
                           or \ Xt
  0b0_01_101_1_0_00000_00000000000000_00000 or
;

\ orr Xd, Xn, #(1 << bit)
\ TODO add general-purpose `orr`.
: asm_orr_bit ( Xd Xn bit -- )
  64 swap - 16 lsl    \ immr
  swap      5  lsl or \ Xn
                   or \ Xd
  0b1_01_100100_1_000000_000000_00000_00000 or
;

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

: @2 ( adr -- val0 val1 ) [
          asm_pop_x1        comp_instr \ ldr x1, [x27, -8]!
  1 2 1 0 asm_load_pair_off comp_instr \ ldp x1, x2, [x1]
          asm_push_x1_x2    comp_instr \ stp x1, x2, [x27], 16
] ;

: ! ( val adr -- ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_off comp_instr \ stur x1, [x2]
] ;

: !2 ( val0 val1 adr -- val0 val1 ) [
  3       asm_pop1           comp_instr \ ldr x3, [x27, -8]!
          asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 3 0 asm_store_pair_off comp_instr \ stp x1, x2, [x3]
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

\ Pushes the address of the next writable stack cell.
\ Like `sp@` in Gforth but renamed to make sense.
\ Note: our integer stack is empty-ascending.
\
\ Should have been `str x27, [x27], 8`, but Arm64 has a quirk where an
\ address-modifying load or store which also uses the address register
\ in the value position is considered to be "unpredictable". Different
\ CPUs are allowed to handle this differently; on Apple Silicon chips,
\ this is considered a "bad instruction" and blows up.
: sp ( -- adr ) [
  ARCH_REG_DAT_SP ARCH_REG_DAT_SP 0 asm_store_off comp_instr \ stur x27, [x27]
  ARCH_REG_DAT_SP ARCH_REG_DAT_SP 8 asm_add_imm   comp_instr \ add x27, x27, 8
] ;

\ Sets the stack pointer register to the given address.
\ Uses two instructions for the same reason as the above.
: sp! ( adr -- ) [
                    asm_pop_x1  comp_instr \ ldr x1, [x27, -8]!
  ARCH_REG_DAT_SP 1 asm_mov_reg comp_instr \ mov x27, x1
] ;

: cell 8 ;
: cells ( len -- size ) 3 lsl ;
: /cells ( size -- len ) 3 asr ;

\ Stack introspection doesn't need to be optimal.
: sp_at       ( ind    -- ptr ) cells sp0 + ;
: stack_len   (        -- len ) sp sp0 - /cells ;
: stack_clear ( …      -- )     sp0 sp! ;
: stack_trunc ( … len  -- … )   sp_at sp! ;
: stack+      ( … diff -- … )   sp swap dec cells + sp! ;
: stack-      ( … diff -- … )   sp swap inc cells - sp! ;

: c@ ( str -- char ) [
        asm_pop_x1        comp_instr \ ldr x1, [x27, -8]!
  1 1 0 asm_load_byte_off comp_instr \ ldrb x1, [x1]
        asm_push_x1       comp_instr \ str x1, [x27], 8
] ;

: c! ( char adr -- ) [
        asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_byte_off comp_instr \ strb x1, [x2]
] ;

:: char' parse_word drop c@ comp_push ;

\ Interpretation semantics of standard `s"`.
\ The parser ensures null-termination.
: parse_str ( E: "str" -- cstr len ) char' " parse ;

\ Compilation semantics of standard `s"`.
: comp_str ( C: "str" -- ) ( E: -- cstr len )
  parse_str tuck                ( len str len )
  inc                           \ Reserve 1 more for null byte.
  alloc_data 1   comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  2              comp_load      \ ldr x2, <len>
  asm_push_x1_x2 comp_instr     \ stp x1, x2, [x27], 16
;

: comp_cstr ( C: "str" -- ) ( E: -- cstr )
  parse_str inc               \ Reserve 1 more for null byte.
  alloc_data 1 comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1  comp_instr     \ str x1, [x27], 8
;

:  " ( E: "str" -- cstr len )           parse_str ;
:: " ( C: "str" -- ) ( E: -- cstr len ) comp_str ;

:  c" ( E: "str" -- cstr )           parse_str drop ;
:: c" ( C: "str" -- ) ( E: -- cstr ) comp_cstr ;

\ ## Variables
\
\ Unlike in ANS Forth, variables take an initial value.
\
\ We could easily define variables without compiler support,
\ allocating memory via libc. The reason for special support
\ such as `alloc_data` is compatibility with AOT compilation
\ which is planned for later.

\ For words which define words. Kinda like `create`.
:: #word_beg compile' : ;
:: #word_end compile'' ; not_comp_only ;

\ Similar to standard `constant`.
: let: ( C: val -- ) ( E: -- val ) #word_beg comp_push #word_end ;

\ Similar to standard `variable`.
: var: ( C: init "name" -- ) ( E: -- addr )
  #word_beg
  sp cell -                  \ Address of the `init` value.
  cell        alloc_data     ( -- addr )
  1           comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr     \ str x1, [x27], 8
  drop                       \ Don't need the `init` value anymore.
  #word_end
;

\ Creates a global variable which refers to a buffer of at least
\ the given size in bytes, rounded up to page size.
\
\ Serves the same role as the standard idiom `create <name> N allot`
\ which we don't have because we segregate code and data.
: buf: ( C: size "name" -- ) ( E: -- addr size )
  #word_beg
  dup 0 swap  alloc_data     ( -- addr )
  1           comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr     \ str x1, [x27], 8
              comp_push      \ str <size>, [x27], 8
  #word_end
;

\ Shortcut for the standard idiom `create <name> N cells allot`.
: cells: ( C: len "name" -- ) ( E: -- addr )
  #word_beg
  cells 0 swap alloc_data     ( -- addr )
  1            comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1  comp_instr     \ str x1, [x27], 8
               comp_push      \ str <size>, [x27], 8
  #word_end
;

0 let: false
1 let: true

\ ## Locals
\
\ When we ask for a local, the compiler returns an FP offset.
\ We can compile load/store and push/pop as we like.

: asm_local_get ( reg fp_off -- instr ) ARCH_REG_FP swap asm_load_scaled_off ;
: asm_local_set ( reg fp_off -- instr ) ARCH_REG_FP swap asm_store_scaled_off ;

\ SYNC[asm_local_read].
: comp_local_get_push ( fp_off -- )
  1 swap asm_local_get comp_instr \ ldr x1, [FP, <loc>]
  1      asm_push1     comp_instr \ str x1, [x27], 8
;

\ SYNC[asm_local_write].
: comp_pop_local_set ( fp_off -- )
  1      asm_pop1      comp_instr \ ldr x1, [x27, -8]!
  1 swap asm_local_set comp_instr \ str x1, [FP, <loc>]
;

:: to: ( C: "name" -- ) ( E: val -- )
  parse_word get_local comp_pop_local_set
;

\ ## Exceptions
\
\ Exception definitions are split. See additional words below
\ which support message formatting via the C "printf" family.
\
\ `x0` is our error register. The annotation `throws` makes the compiler
\ insert an error check after each call to this procedure and any other
\ procedure which calls this one, turning the error into an exception.
\ The `len` parameter is here because buffer variables return it.
: throw ( cstr len -- )
  [ throws ]
  drop
  [ 0 asm_pop1 comp_instr ] \ ldr x0, [x27, -8]!
;

\ Usage: `throw" some_error_msg"`.
\
\ Also see `sthrowf"` for formatted errors.
:: throw" ( C: "str" -- ) comp_str compile' throw ;

\ ## Memory
\
\ The program is implicitly linked with `libc`.
\ We're free to use Posix-standard C functions.
\ C variables like `stdout` are a bit trickier,
\ but also optional; we could `fdopen` instead.

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

: move    ( buf[src] buf[tar] len -- ) swap_over strncpy ;
: fill    ( buf len char -- ) swap memset ;
: erase   ( buf len -- ) 0 fill ;
: blank   ( buf len -- ) 32 fill ;
: compare ( str0 len0 str1 len1 -- direction ) rot min strncmp ;
: str=    ( str0 len0 str1 len1 -- bool ) compare =0 ;
: str<    ( str0 len0 str1 len1 -- bool ) compare <0 ;
: str<>   ( str0 len0 str1 len1 -- bool ) compare <>0 ;

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
\ Unlike other Forth systems, we also support chains
\ of "elif" terminated with a single "end".

: pc_off ( adr -- off ) here swap - ; \ For forward jumps.
: reserve_instr ASM_PLACEHOLDER comp_instr ;
: reserve_cond asm_pop_x1 comp_instr here reserve_instr ;

\ b <pc_off>
: patch_uncond_forward ( adr -- ) dup pc_off asm_branch swap !32 ;

\ b -<pc_off>
: patch_uncond_back ( adr -- ) here - asm_branch swap !32 ;

0 var: COND_EXISTS

\ cbz x1, <else|end>
: if_patch ( adr[if] -- ) dup pc_off 1 swap asm_cmp_branch_zero swap !32 ;

\ cbnz x1, <else|end>
: ifn_patch ( adr[ifn] -- ) dup pc_off 1 swap asm_cmp_branch_not_zero swap !32 ;

: if_done ( cond_prev -- ) COND_EXISTS ! ;
: if_pop ( fun[prev] adr[if]  -- ) if_patch execute ;
: ifn_pop ( fun[prev] adr[ifn]  -- ) ifn_patch execute ;
: if_init COND_EXISTS @ COND_EXISTS on! ' if_done ;

:: #elif ( C: -- fun[exec] adr[elif] fun[elif] ) ( E: pred -- )
  ' execute reserve_cond ' if_pop
;

:: #elifn ( C: -- fun[exec] adr[elif] fun[elif] ) ( E: pred -- )
  ' execute reserve_cond ' ifn_pop
;

\ With this strange setup, `#end` pops the top control frame,
\ which pops the next control frame, and so on. This allows us
\ to pop any amount of control constructs with a single `#end`.
\ Prepending a nop terminates this chain.
:: #if ( C: -- prev fun[done] fun[exec] adr[if] fun[if] ) ( E: pred -- )
  if_init execute'' #elif
;

:: #ifn ( C: -- fun[nop] fun[exec] adr[ifn] fun[ifn] ) ( E: pred -- )
  if_init execute'' #elifn
;

: else_pop ( fun[prev] adr[else] -- ) patch_uncond_forward execute ;

:: #else
  ( C: fun[exec] adr[if] fun[if] -- adr[else] fun[else] )
  here to: adr reserve_instr

  \ Turn `fun[exec]` into `fun[nop]` to prevent "if" from chaining.
  ' nop 2 bury

  execute
  adr ' else_pop
;

\ TODO: replace `'` in control structures with raw native code pointers,
\ and replace `execute` with a new word, let's say `call`, which invokes
\ an instruction address and checks the error register. That's about 3-5
\ lines in Forth, 3 instructions total. This should reduce or remove the
\ overhead of calling compiler intrinsics in control chains.
:: #end ( C: fun -- ) execute ;

\ ## Loops
\
\ Unlike other Forth systems, we implement termination of arbitrary
\ control structures with a generic `#end`. This works for all loops
\ and eliminates the need to remember many different terminators.
\ The added internal complexity is manageable.

\ Stack position of a special location in the top loop frame on the control
\ stack. This is where auxiliary structures such as `#leave` place their own
\ control frames.
0 var: LOOP_FRAME

: loop_frame! ( frame_ind -- ) LOOP_FRAME ! ;

: loop_frame_beg ( -- frame_ind fun )
  LOOP_FRAME @ ' loop_frame! stack_len LOOP_FRAME !
;

\ Used by auxiliary loop constructs such as "leave" and "while"
\ to insert their control frame before a loop control frame.
\ This allows to pop the control frames in the right order
\ with a single "end" word.
: loop_aux ( prev… <loop> …rest… <frame> stack_len -- prev… <frame> <loop> …rest )
            to: stack_len_0
  stack_len to: stack_len_1

  LOOP_FRAME @
  #ifn
    throw" auxiliary loop constructs require an ancestor loop frame"
  #end

  stack_len_1  stack_len_0 - to: frame_len
  frame_len    cells         to: frame_size
  stack_len_0  sp_at         to: frame_sp_prev
  stack_len_1  sp_at         to: frame_sp_temp
  LOOP_FRAME @ sp_at         to: loop_sp_prev
  loop_sp_prev frame_size  + to: loop_sp_next

  \ Need a backup copy of the frame we're moving,
  \ because we're about to overwrite that data.
  frame_len stack+
  frame_sp_temp frame_sp_prev frame_size memmove

  \ Move everything starting with the loop frame,
  \ making space for the aux frame we're inserting.
  frame_sp_temp loop_sp_next - to: rest_size
  loop_sp_next loop_sp_prev rest_size memmove

  \ The aux frame is now behind the moved loop frame.
  loop_sp_prev frame_sp_temp frame_size memmove
  frame_len stack-

  \ Subsequent aux constructs will be inserted after the new frame.
  LOOP_FRAME @ frame_len + LOOP_FRAME !
;

: loop_end ( adr[beg] -- ) here - asm_branch comp_instr ; \ b <begin>
: loop_pop ( fun[prev] …aux… adr[beg] -- ) loop_end execute'' #end ;

:: #loop
  ( C: -- frame fun[frame!] … adr[beg] fun[loop] )
  \ Control frame is split: ↑ auxiliary constructs like "leave" go here.
  loop_frame_beg here ' loop_pop
;

\ Can be used in any loop.
:: #leave
  ( C: prev… <loop> …rest -- prev… adr[leave] fun[leave] <loop> …rest )
  stack_len to: len
  here reserve_instr ' else_pop len loop_aux
;

\ Can be used in any loop.
:: #while
  ( C: prev… <loop> …rest -- prev… adr[while] fun[while] <loop> …rest )
  stack_len to: len
  reserve_cond ' if_pop len loop_aux
;

\ Assumes that the top control frame is from `#loop`.
:: #until ( C: prev… <loop> -- )
      asm_pop_x1          comp_instr \ ldr x1, [x27, -8]!
  1 8 asm_cmp_branch_zero comp_instr \ cbnz x1, 8
  execute
;

: for_loop_pop ( fun[prev] …aux… adr[cond] adr[beg] )
  loop_end \ b <begin>
  if_patch \ Retropatch loop condition.
  execute  \ Pop auxiliaries; restore previous loop frame.
;

: for_loop_init
  ( C: local_ind -- frame fun[frame!] … adr[cond] adr[beg] fun[for] )
  ( E: limit -- )

  to: index
  loop_frame_beg
  index comp_pop_local_set \ Reuse limit as index.

  here to: adr_beg
  1 index asm_local_get comp_instr    \ ldur x1, [FP, <loc_off>]
  here to: adr_cond     reserve_instr \ cbz x1, <end>
  1 1 1   asm_sub_imm   comp_instr    \ sub x1, x1, 1
  1 index asm_local_set comp_instr    \ stur x1, [FP, <loc_off>]

  adr_cond adr_beg ' for_loop_pop
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
  ( C: -- frame fun[frame!] … adr[cond] adr[beg] fun[pop] )
  ( E: ceil -- )
  anon_local for_loop_init
;

\ Like `#for` but requires a local name to make the index
\ accessible. Usage; ceiling must be positive:
\
\   123
\   -for: ind
\     ind .
\   #end
:: -for:
  ( C: "name" -- frame fun[frame!] … adr[cond] adr[beg] fun[pop] )
  ( E: ceil -- )
  parse_word get_local for_loop_init
;

\ Not used by `#for` and `-for:` because they get away
\ with storing just one local instead of two.
\
\ The meaning of "cursor" and "limit" may differ between loop words.
\ The cursor is often a pointer.
: comp_count_loop_beg ( loc_cur loc_lim -- adr[cond] adr[beg] loc_cur )
  to: limit
  to: cursor

  here to: adr_beg
  1 cursor asm_local_get comp_instr \ ldr x1, [FP, <loc>]
  2 limit  asm_local_get comp_instr \ ldr x2, [FP, <loc>]
  1 2      asm_cmp_reg   comp_instr \ cmp x1, x2

  here to: adr_cond
  reserve_instr \ b.cond <end>

  adr_cond adr_beg cursor
;

: count_loop_pop ( fun[prev] …aux… adr[cond] adr[beg] asm_cond -- )
  to: asm_cond
  loop_end \ b <begin>
  to: adr_cond
  asm_cond adr_cond pc_off asm_branch_cond adr_cond !32 \ b.cond <end>
  execute \ Pop auxiliaries; restore previous loop frame.
;

: count_up_loop_pop ASM_GE count_loop_pop ;
: count_down_loop_pop ASM_LT count_loop_pop ;

: plus_for_loop_pop ( fun[prev] …aux… adr[cond] adr[beg] loc_cur )
  to: cursor
  1 cursor asm_local_get comp_instr \ ldur x1, [FP, <loc>]
  1 1 1    asm_add_imm   comp_instr \ add x1, x1, 1
  1 cursor asm_local_set comp_instr \ stur x1, [FP, <loc>]
  count_up_loop_pop
;

\ A count-up-by-one loop. Analogue of the standard `?do … loop` idiom.
\ Requires a name to make the index accessible. Usage:
\
\   123 34
\   +for: ind
\     ind .
\   #end
:: +for:
  ( C: "name" -- frame fun[frame!] … adr[cond] adr[beg] loc_cur fun[pop] )
  ( E: ceil floor -- )
  loop_frame_beg
  parse_word

  anon_local to: ceil
  get_local  to: floor \ Reuse as cursor / index.

  floor      comp_pop_local_set
  ceil       comp_pop_local_set
  floor ceil comp_count_loop_beg
  ' plus_for_loop_pop
;

: plus_loop_pop ( fun[prev] …aux… adr[cond] adr[beg] loc_cur loc_step )
  to: step to: cursor

  1 cursor asm_local_get comp_instr \ ldr x1, [FP, <loc>]
  2 step   asm_local_get comp_instr \ ldr x2, [FP, <loc>]
  1 1 2    asm_add_reg   comp_instr \ add x1, x1, x2
  1 cursor asm_local_set comp_instr \ str x1, [FP, <off>]
  count_up_loop_pop
;

\ Similar to the standard `?do ... +loop`, but terminated with `#end`
\ like other loops. Takes a name to make the index accessible. Usage:
\
\   ceil floor step +loop: ind ind . #end
\   123  23    3    +loop: ind ind . #end
:: +loop:
  ( C: "name" -- frame fun[frame!] … adr[cond] adr[beg] loc_cur loc_step fun[pop] )
  ( E: ceil floor step -- )
  loop_frame_beg

  parse_word
  anon_local to: ceil
  get_local  to: floor \ Reuse as cursor.
  anon_local to: step

  step       comp_pop_local_set
  floor      comp_pop_local_set
  ceil       comp_pop_local_set
  floor ceil comp_count_loop_beg
  step ' plus_loop_pop
;

\ Like in `+loop:`, the range is `[floor,ceil)`.
:: -loop:
  ( C: "name" -- frame fun[frame!] … adr[cond] adr[beg] fun[pop] )
  ( E: ceil floor step -- )
  loop_frame_beg

  parse_word
  get_local  to: ceil \ Reuse as cursor.
  anon_local to: floor
  anon_local to: step

  step  comp_pop_local_set
  floor comp_pop_local_set
  ceil  comp_pop_local_set

  here to: adr_beg

  \ We can almost use `comp_count_loop_beg` here,
  \ but this loop wants subtraction at the start.
  \ TODO use `ldp` for adjacent offsets.
  1 ceil  asm_local_get comp_instr    \ ldur x1, [FP, <ceil_off>]
  2 floor asm_local_get comp_instr    \ ldur x2, [FP, <floor_off>]
  3 step  asm_local_get comp_instr    \ ldur x3, [FP, <step_off>]
  1 1 3   asm_sub_reg   comp_instr    \ sub x1, x1, x3
  1 2     asm_cmp_reg   comp_instr    \ cmp x1, x2
  here to: adr_cond     reserve_instr \ b.cond <end>
  1 ceil  asm_local_set comp_instr    \ stur x1, [FP, <ceil_off>]

  adr_cond adr_beg ' count_down_loop_pop
;

: ?dup ( val bool -- val ?val ) #if dup #end ;

\ Finds an extern symbol by name and creates a new word which loads the address
\ of that symbol. The extern symbol can be either a variable or a procedure.
: extern_ptr: ( C: "name" str len -- ) ( E: -- ptr )
  #word_beg
  parse_word extern_got to: adr \ Addr of GOT entry holding extern addr.
  adr 1       comp_page_load    \ `adrp x1, <page>` & `ldr x1, x1, <pageoff>`
  asm_push_x1 comp_instr        \ str x1, [x27], 8
  #word_end
;

\ Like `extern_ptr:` but with an additional load instruction.
\ Analogous to extern vars in C. Lets us access stdio files.
: extern_val: ( C: "name" str len -- ) ( E: -- val )
  #word_beg
  parse_word extern_got to: adr     \ Addr of GOT entry holding extern addr.
  adr 1              comp_page_load \ `adrp x1, <page>` & `ldr x1, x1, <pageoff>`
  1 1 0 asm_load_off comp_instr     \ ldur x1, [x1]
        asm_push_x1  comp_instr     \ str x1, [x27], 8
  #word_end
;

extern_val: stdin  __stdinp
extern_val: stdout __stdoutp
extern_val: stderr __stderrp

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
: lf 10 putchar ; \ Renamed from `cr` which would be a misnomer.
: elf 10 eputchar ;
: space 32 putchar ;
: espace 32 eputchar ;

:  log" parse_str drop puts ;
:: log" comp_cstr compile' puts ;

:  elog" parse_str drop eputs ;
:: elog" comp_cstr compile' eputs ;

\ Compiles movement of values from the data stack
\ to the locals in the intuitive "reverse" order.
: } ( locals… len -- ) #for comp_pop_local_set #end ;

\ Support for the `{ inp0 inp1 -- out0 out1 }` locals notation.
\ Capture order matches stack push order in the parens notation
\ like in Gforth and VfxForth. Types are not supported.
:: {
  0 to: loc_len

  #loop
    parse_word to: len to: str
    " }" str len str=
    #if loc_len } #ret #end

    " --" str len str<> #while \ `--` to `}` is a comment.

    \ Popped / used by `}` which we're about to call.
    str len get_local

    loc_len inc to: loc_len
  #end

  \ After `--`: skip all words which aren't `}`.
  #loop parse_word " }" str<> #until

  loc_len }
;

\ For compiling words which modify a local by applying the given function.
: mut_local ( C: fun "name" -- ) ( E: -- )
  parse_word get_local
  { fun loc }
  loc comp_local_get_push
  fun comp_call
  loc comp_pop_local_set
;

\ For compiling words which modify a local by applying the given function.
:: mut_local' ( C: "name" fun -- ) ( E: -- )
  execute'' '
  compile' mut_local
;

\ Usage: `++: some_local`.
:: ++: ( C: "name" -- ) ( E: -- ) mut_local' inc ;
:: --: ( C: "name" -- ) ( E: -- ) mut_local' dec ;

: within { num one two -- bool }
  one num >
  num two <=
  =0 or =0
;

: align_down ( size width -- size ) negate and ;
: align_up   { size width -- size } width dec size + width align_down ;

\ On Arm64, we can't just modify `sp` by 8 bytes and leave it like that.
\ The ABI requires it to be aligned to 16 bytes when loading or storing.
\ We duplicate-store to have a predictable value in the upper 8 bytes,
\ instead of leaving random stack garbage there.
\
\ TODO better name: this pops one stack item, but `dup` implies otherwise.
: dup>systack [
  inline
                     asm_pop_x1         comp_instr \ ldr x1, [x27, -8]!
  1 1 ARCH_REG_SP -16 asm_store_pair_pre comp_instr \ stp x1, x1, [sp, -16]!
] ;

: pair>systack ( Forth: i1 i2 -- | System: -- i1 i2 ) [
  inline
                     asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 ARCH_REG_SP -16 asm_store_pair_pre comp_instr \ stp x1, x2, [sp, -16]!
] ;

\ Note: `add` is one of the few instructions which treat `x31` as `sp`
\ rather than `xzr`. `add x1, sp, 0` disassembles as `mov x1, sp`.
: systack_ptr ( -- sp ) [
  inline
  1 ARCH_REG_SP 0 asm_add_imm comp_instr \ add x1, sp, 0
                 asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: asm_comp_systack_push ( len -- )
  dup <=0 #if drop #ret #end

  dup odd #if
    ' dup>systack inline_word
    dec
  #end

  dup #ifn drop #ret #end

  #loop
    dup #while
    ' pair>systack inline_word
    2 -
  #end
  drop
;

: asm_comp_systack_pop ( len -- )
  dup <=0 #if #ret #end
  cells 16 align_up
  \ add sp, sp, <size>
  ARCH_REG_SP ARCH_REG_SP rot asm_add_imm comp_instr
;

\ Short for "varargs". Sets up arguments for a variadic call.
\ Assumes the Apple Arm64 ABI where varargs use the systack.
\ Usage:
\
\   : some_word
\     #c" numbers: %zd %zd %zd"
\     10 20 30 [ 3 va- ] printf [ -va ] lf
\   ;
\
\ Caution: varargs can only be used in direct calls to variadic procedures.
\ Indirect calls DO NOT WORK because the stack pointer is changed by calls.
: va- ( C: len -- len ) ( E: <systack_push> ) dup asm_comp_systack_push ;
: -va ( C: len -- )     ( E: <systack_pop> )      asm_comp_systack_pop ;

\ Format-prints to stdout using `printf`. `N` is the variadic arg count,
\ which must be available at compile time. Usage example:
\
\   10 20 30 [ 3 ] logf" numbers: %zu %zu %zu" lf
:: logf" ( C: N -- ) ( E: i1 … iN -- )
  va- execute'' c" compile' printf -va
;

\ Format-prints to stderr.
:: elogf" ( C: N -- ) ( E: i1 … iN -- )
  va- compile' stderr execute'' c" compile' fprintf -va
;

\ Formats into the provided buffer using `snprintf`. Usage example:
\
\   SOME_BUF 10 20 30 [ 3 ] sf" numbers: %zu %zu %zu" lf
:: sf" ( C: N -- ) ( E: buf size i1 … iN -- )
  va- comp_cstr compile' snprintf -va
;

\ Formats an error message into the provided buffer using `snprintf`,
\ then throws the buffer as the error value. The buffer must be zero
\ terminated; `buf:` ensures this automatically. Usage example:
\
\   4096 buf: SOME_BUF
\   SOME_BUF 10 20 30 [ 20 ] sthrowf" error codes: %zu %zu %zu"
\
\ Also see `throwf"` which comes with its own buffer.
:: sthrowf" ( C: len -- ) ( E: buf size i1 … iN -- )
  va-
  compile'  dup2
  comp_cstr
  compile'  snprintf
  -va
  compile'  throw
;

4096 buf: ERR_BUF

\ Like `sthrowf"` but easier to use. Example:
\
\   10 20 30 [ 20 ] throwf" error codes: %zu %zu %zu"
:: throwf" ( C: len -- ) ( E: i1 … iN -- )
  va-
  compile'  ERR_BUF
  comp_cstr
  compile'  snprintf
  -va
  compile' ERR_BUF
  compile' throw
;

\ Similar to standard `abort"`, with clearer naming.
:: throw_if" ( C: "str" -- ) ( E: pred -- )
  execute'' #if
  comp_str
  compile' throw
  execute'' #end
;

: log_int ( num -- ) [ 1 ] logf" %zd"  ;
: log_cell ( num ind -- ) dup_over [ 3 ] logf" %zd 0x%zx <%zd>" ;
: . ( num -- ) stack_len dec log_cell lf ;

: .s
  stack_len to: len

  len #ifn
    log" stack is empty" lf
    #ret
  #end

  len <0 #if
    log" stack length is negative: " len log_int lf
    #ret
  #end

  len [ 1 ] logf" stack <%zd>:" lf

  0
  #loop
    dup to: ind
    ind len < #while
    space space
    ind pick0 ind log_cell lf
    inc
  #end
  drop
;

: .sc .s stack_clear ;

extern_val: errno __error
1 1 extern: strerror
6 1 extern: mmap
3 1 extern: mprotect

16384 let: PAGE_SIZE
0     let: PROT_NONE
1     let: PROT_READ
2     let: PROT_WRITE
2     let: MAP_PRIVATE
4096  let: MAP_ANON

: mem_map_err ( -- )
  errno dup strerror
  [ 2 ] throwf" unable to map memory; code: %d; message: %s"
;

: mem_map { size pflag -- addr }
  MAP_ANON MAP_PRIVATE or to: mflag
  0 size pflag mflag -1 0 mmap
  dup -1 = #if drop mem_map_err #end
;

: mem_unprot_err ( -- )
  errno dup strerror
  [ 2 ] throwf" unable to unprotect memory; code: %d; message: %s"
;

: mem_unprot ( addr size -- )
  PROT_READ PROT_WRITE or mprotect
  -1 = #if throw" unable to mprotect" #end
;

\ Allocates a guarded buffer: `guard|data|guard`.
\ Attempting to read or write inside the guards,
\ aka underflow or overflow, triggers a segfault.
\ The given size is rounded up to the page size.
: mem_alloc ( size1 -- addr size2 )
        PAGE_SIZE align_up to: size2
  size2 PAGE_SIZE 1 lsl +  to: size
  size  PROT_NONE mem_map  to: addr
  addr  PAGE_SIZE +        to: addr
  addr  size2 mem_unprot
  addr  size2
;

\ Calling this "guarded" is an overstatement. The size gets page-aligned,
\ which often means there's an unguarded upper region which is writable
\ and readable and allows overflows. However, underflows into the lower
\ guard are successfully prevented.
\
\ TODO consider compiling with lazy-init; dig up the old code.
: cells_guarded: ( C: len "name" -- ) ( E: -- addr )
  #word_beg
  cells mem_alloc drop
  1           comp_load  \ ldr x1, <addr>
  1 asm_push1 comp_instr \ str x2, [x27], 8
  #word_end
;
