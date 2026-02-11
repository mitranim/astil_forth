:  drop2 [ 0b1_1_0_100010_0_000000010000_11011_11011 comp_instr ] ;
:: \ 10 parse drop2 ;
:: ( 41 parse drop2 ;

\ Can use comments now!
\
\ This file boostraps the Forth language. The outer interpreter / compiler
\ provides only the most fundamental intrinsics; enough for self-assembly.
\ We define basic words in terms of machine instructions, building up from
\ there. Currently only the Arm64 CPU architecture is supported.
\
\ The compiler and language come in two variants: stack-CC and register-CC.
\ This file uses the traditional stack-based calling convention. See the
\ file `./lang_r.f` for the register-based calling convention.

\ brk 666
: abort [ 0b110_101_00_001_0000001010011010_000_00 comp_instr ] ;
: unreachable abort ;
: nop ;

\ ## Assembler and bitwise ops
\
\ We interleave definitions of assembler words with definitions
\ of various arithmetic words used by the assembler.
\
\ The assembler is also split; some parts are defined
\ further down, after some stack-manipulation words.

: ASM_INSTR_SIZE       4      ;
: ASM_REG_DAT_SP_FLOOR 26     ;
: ASM_REG_DAT_SP       27     ;
: ASM_REG_INTERP       28     ;
: ASM_REG_FP           29     ;
: ASM_REG_LR           30     ;
: ASM_REG_SP           31     ;
: ASM_EQ               0b0000 ;
: ASM_NE               0b0001 ;
: ASM_CS               0b0010 ;
: ASM_CC               0b0011 ;
: ASM_MI               0b0100 ;
: ASM_PL               0b0101 ;
: ASM_VS               0b0110 ;
: ASM_VC               0b0111 ;
: ASM_HI               0b1000 ;
: ASM_LS               0b1001 ;
: ASM_GE               0b1010 ;
: ASM_LT               0b1011 ;
: ASM_GT               0b1100 ;
: ASM_LE               0b1101 ;
: ASM_AL               0b1110 ;
: ASM_NV               0b1111 ;
: ASM_PLACEHOLDER      666    ; \ udf 666; used in retropatching.

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

\ cmp Xn, <imm>
: asm_cmp_imm ( Xn imm -- instr )
       10 lsl    \ imm
  swap 5  lsl or \ Xn
  0b1_1_1_100010_0_000000000000_00000_11111 or
;

\ cmp Xn, 0
: asm_cmp_zero ( Xn -- instr ) 0 asm_cmp_imm ;

\ cset Xd, <cond>
: asm_cset ( Xd cond -- instr )
  0b1_0_0_11010100_11111_0000_0_1_11111_00000
  swap 1 xor 12 lsl or \ cond
                    or \ Xd
;

: <>0 ( int -- bool ) [
           asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
  1        asm_cmp_zero comp_instr \ cmp x1, 0
  1 ASM_NE asm_cset     comp_instr \ cset x1, ne
           asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: =0 ( int -- bool ) [
           asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
  1        asm_cmp_zero comp_instr \ cmp x1, 0
  1 ASM_EQ asm_cset     comp_instr \ cset x1, eq
           asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

\ Shared by a lot of load and store instructions. 64-bit only.
: asm_pattern_load_store_pair ( Xt1 Xt2 Xn imm7 -- instr_mask )
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
: asm_pattern_load_store ( Xt Xn imm9 -- instr_mask )
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
: asm_pattern_load_store_with_register ( Xt Xn Xm scale -- instr_mask )
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
: asm_push1 ( Xd -- instr ) ASM_REG_DAT_SP 8 asm_store_post ;

\ ldr Xd, [x27, -8]!
: asm_pop1 ( Xd -- instr ) ASM_REG_DAT_SP -8 asm_load_pre ;

\ stp Xt1, Xt2, [x27], 16
: asm_push2 ( Xt1 Xt2 -- instr ) ASM_REG_DAT_SP 16 asm_store_pair_post ;

\ ldp Xt1, Xt2, [x27, -16]!
: asm_pop2 ( Xt1 Xt2 -- instr ) ASM_REG_DAT_SP -16 asm_load_pair_pre ;

\ neg Xd, Xd
: asm_neg ( Xd -- instr )
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

: asm_pattern_arith_csel ( Xd Xn Xm cond -- instr_mask )
  [ 3 asm_pop1 comp_instr ]  \ Stash `cond`.
  asm_pattern_arith_reg      \ Xd Xm Xn
  [ 3 asm_push1 comp_instr ] \ Pop `cond`.
  12 lsl or                  \ cond
;

\ csel Xd, Xn, Xm, <cond>
: asm_csel ( Xd Xn Xm cond -- instr )
  asm_pattern_arith_csel
  0b1_0_0_11010100_00000_0000_0_0_00000_00000 or
;

\ csneg Xd, Xn, Xm, <cond>
: asm_csneg ( Xd Xn Xm cond -- instr )
  asm_pattern_arith_csel
  0b1_1_0_11010100_00000_0000_0_1_00000_00000 or
;

\ cmp Xn, Xm
: asm_cmp_reg ( Xn Xm -- instr )
       16 lsl    \ Xm
  swap 5  lsl or \ Xn
  0b1_1_1_01011_00_0_00000_000_000_00000_11111 or
;

\ b <off>
: asm_branch ( off26 -- instr )
  2  lsr       \ Offset is implicitly times 4.
  26 bit_trunc \ Offset may be negative.
  0b0_00_101_00000000000000000000000000 or
;

\ b.<cond> <off>
: asm_branch_cond ( off19 cond -- instr )
  swap 2 asr 19 bit_trunc 5 lsl \ `off19`; implicitly times 4.
  or                            \ cond
  0b010_101_00_0000000000000000000_0_0000 or
;

\ `off19` offset is implicitly times 4 and can be negative.
: asm_pattern_cmp_branch ( Xt off19 -- instr_mask )
  2 lsr 19 bit_trunc 5 lsl \ off19
                        or \ Xt
;

\ cbz x1, <off>
: asm_cmp_branch_zero ( Xt off19 -- instr )
  asm_pattern_cmp_branch
  0b1_011010_0_0000000000000000000_00000 or
;

\ cbnz x1, <off>
: asm_cmp_branch_not_zero ( Xt off19 -- instr )
  asm_pattern_cmp_branch
  0b1_011010_1_0000000000000000000_00000 or
;

\ Same as `0 pick`.
: dup ( i1 -- i1 i1 ) [
  1 ASM_REG_DAT_SP -8 asm_load_off comp_instr \ ldur x1, [x27, -8]
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

\ ## Compilation-related words
\
\ These words are used for metaprogramming
\ and mostly rely on compiler intrinsics.
\
\ Unlike in standard Forth, we use separate wordlists for
\ regular and compile-time / immediate words. As a result,
\ we have to define parsing words like "compile" in pairs:
\ one tick seeks in the regular wordlist, and double tick
\ seeks in the compile-time wordlist.

\ SYNC[wordlist_enum].
: WORDLIST_EXEC 1 ;
: WORDLIST_COMP 2 ;

\ Non-immediate replacement for standard `literal`.
: comp_push ( C: num -- ) ( E: -- num )
  1           comp_load  \ ldr x1, <num>
  asm_push_x1 comp_instr \ str x1, [x27], 8
;

: next_word ( wordlist "word" -- exec_tok ) parse_word find_word ;

:  tick_next ( C: wordlist "word" -- ) ( E: -- exec_tok ) next_word comp_push ;
:: '  WORDLIST_EXEC tick_next ;
:: '' WORDLIST_COMP tick_next ;

:  inline_next ( wordlist "word" -- ) ( E: <word> ) next_word inline_word ;
:: inline'  WORDLIST_EXEC inline_next ;
:: inline'' WORDLIST_COMP inline_next ;

\ "execute" is renamed from standard "postpone".
:  execute_next ( wordlist "word" -- ) ( E: <word> ) next_word comp_call ;
:: execute'  WORDLIST_EXEC execute_next ;
:: execute'' WORDLIST_COMP execute_next ;

:  compile_next ( wordlist "word" -- ) next_word comp_push ' comp_call comp_call ;
:: compile'  WORDLIST_EXEC compile_next ;
:: compile'' WORDLIST_COMP compile_next ;

\ ## Stack manipulation

: drop ( val -- ) [
  ASM_REG_DAT_SP ASM_REG_DAT_SP 8 asm_sub_imm comp_instr \ sub x27, x27, 8
] ;

\ Same as `1 pick 1 pick`.
: dup2 ( i1 i2 -- i1 i2 i1 i2 ) [
  1 2 ASM_REG_DAT_SP -16 asm_load_pair_off comp_instr \ ldp x1, x2, [x27, -16]
  1 2                    asm_push2         comp_instr \ stp x1, x2, [x27], 16
] ;

\ Same as `swap drop`.
: nip ( i1 i2 -- i2 ) [
                      asm_pop_x1    comp_instr \ ldr x1, [x27, -8]!
  1 ASM_REG_DAT_SP -8 asm_store_off comp_instr \ stur x1, [x27, -8]
] ;

\ Same as `swap2 drop2`.
: nip2 ( i1 i2 i3 i4 -- i3 i4 ) [
                         asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 ASM_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Same as `1 pick`.
: over ( i1 i2 -- i1 i2 i1 ) [
  1 ASM_REG_DAT_SP -16 asm_load_off comp_instr \ ldur x1, [x27, -16]
                       asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

\ Same as `3 pick 3 pick`.
: over2 ( i1 i2 i3 i4 -- i1 i2 i3 i4 i1 i2 ) [
  1 2 ASM_REG_DAT_SP -32 asm_load_pair_off comp_instr \ ldp x1, x2, [x27, -32]
                         asm_push_x1_x2    comp_instr \ stp x1, x2, [x27], 16
] ;

\ Same as `3 roll 3 roll`.
: swap2 ( i1 i2 i3 i4 -- i3 i4 i1 i2 ) [
  1 2 ASM_REG_DAT_SP -32 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -32]
  3 4 ASM_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x3, x4, [x27, -16]
  3 4 ASM_REG_DAT_SP -32 asm_store_pair_off comp_instr \ stp x3, x4, [x27, -32]
  1 2 ASM_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Same as `-rot swap rot`.
: swap_over ( i1 i2 i3 -- i2 i1 i3 ) [
  1 2 ASM_REG_DAT_SP -24 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -24]
  2 1 ASM_REG_DAT_SP -24 asm_store_pair_off comp_instr \ stp x2, x1, [x27, -24]
] ;

\ Same as `over -rot`.
: dup_over ( i1 i2 -- i1 i1 i2 ) [
  1 2            ASM_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -16]
  1 2            ASM_REG_DAT_SP -8  asm_store_pair_off comp_instr \ stp x1, x2, [x27, -8]
  ASM_REG_DAT_SP ASM_REG_DAT_SP  8  asm_add_imm        comp_instr \ add x27, x27, 8
] ;

\ Same as `dup rot`.
: tuck ( i1 i2 -- i2 i1 i2 ) [
  1 2 ASM_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -16]
  2 1 ASM_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x2, x1, [x27, -16]
                      2  asm_push1          comp_instr \ str x2, [x27], 8
] ;

: tuck2 ( i1 i2 i3 i4 -- i3 i4 i1 i2 i3 i4 ) [
  inline' swap2
  3 4 asm_push2 comp_instr \ stp x3, x4, [x27], 16
] ;

\ Same as `2 roll`.
: rot ( i1 i2 i3 -- i2 i3 i1 ) [
    1 ASM_REG_DAT_SP -24 asm_load_off       comp_instr \ ldur x1, [x27, -24]
  2 3 ASM_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x2, x3, [x27, -16]
  2 3 ASM_REG_DAT_SP -24 asm_store_pair_off comp_instr \ stp x2, x3, [x27, -24]
    1 ASM_REG_DAT_SP  -8 asm_store_off      comp_instr \ stur x1, [x27, -8]
] ;

: -rot ( i1 i2 i3 -- i3 i1 i2 ) [
    1 ASM_REG_DAT_SP -24 asm_load_off       comp_instr \ ldur x1, [x27, -24]
  2 3 ASM_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x2, x3, [x27, -16]
    3 ASM_REG_DAT_SP -24 asm_store_off      comp_instr \ stur x3, [x27, -24]
  1 2 ASM_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

: rot2 ( i1 i2 i3 i4 i5 i6 -- i3 i4 i5 i6 i1 i2 ) [
  1 2 ASM_REG_DAT_SP -48 asm_load_pair_off  comp_instr \ ldp x1, x2, [x27, -48]
  3 4 ASM_REG_DAT_SP -32 asm_load_pair_off  comp_instr \ ldp x3, x4, [x27, -32]
  5 6 ASM_REG_DAT_SP -16 asm_load_pair_off  comp_instr \ ldp x5, x6, [x27, -16]
  3 4 ASM_REG_DAT_SP -48 asm_store_pair_off comp_instr \ stp x3, x4, [x27, -48]
  5 6 ASM_REG_DAT_SP -32 asm_store_pair_off comp_instr \ stp x5, x6, [x27, -32]
  1 2 ASM_REG_DAT_SP -16 asm_store_pair_off comp_instr \ stp x1, x2, [x27, -16]
] ;

\ Pushes the stack item found at the given index, duplicating it.
: pick ( … ind -- … [ind] ) [
                       asm_pop_x1             comp_instr \ ldr x1, [x27, -8]!
  1 1                  asm_mvn                comp_instr \ mvn x1, x1
  1 ASM_REG_DAT_SP 1 1 asm_load_with_register comp_instr \ ldr x1, [x27, x1, lsl 3]
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
  2 2                  asm_mvn                 comp_instr \ mvn x2, x2
  1 ASM_REG_DAT_SP 2 1 asm_store_with_register comp_instr \ str x1, [x27, x2, lsl 3]
] ;

: flip ( i1 i2 i3 -- i3 i2 i1 ) [
  1 ASM_REG_DAT_SP -24 asm_load_off  comp_instr \ ldr x1, [x27, -24]
  2 ASM_REG_DAT_SP -8  asm_load_off  comp_instr \ ldr x2, [x27, -8]
  2 ASM_REG_DAT_SP -24 asm_store_off comp_instr \ str x2, [x27, -24]
  1 ASM_REG_DAT_SP -8  asm_store_off comp_instr \ str x1, [x27, -8]
] ;

\ ## Arithmetic

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
  1            asm_cmp_zero comp_instr \ cmp x1, 0
  1 1 1 ASM_PL asm_csneg    comp_instr \ cneg x1, x1, pl
               asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: min ( i1 i2 -- i1|i2 ) [
               asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
    1 2        asm_cmp_reg   comp_instr \ cmp x1, x2
  1 1 2 ASM_LT asm_csel      comp_instr \ csel x1, x1, x2, lt
               asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: max ( i1 i2 -- i1|i2 ) [
               asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
    1 2        asm_cmp_reg   comp_instr \ cmp x1, x2
  1 1 2 ASM_GT asm_csel      comp_instr \ csel x1, x1, x2, gt
               asm_push_x1   comp_instr \ str x1, [x27], 8
] ;

: cell 8 ;
: cells  ( len  -- size ) 3 lsl ;
: /cells ( size -- len  ) 3 asr ;
: +cell  ( adr  -- adr  ) cell + ;
: -cell  ( adr  -- adr  ) cell - ;

\ ## Assembler continued

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
: asm_orr_bit ( Xd Xn bit -- instr )
  64 swap - 16 lsl    \ immr
  swap      5  lsl or \ Xn
                   or \ Xd
  0b1_01_100100_1_000000_000000_00000_00000 or
;

: asm_comp_cset_reg ( C: cond -- ) ( E: i1 i2 -- bool )
  1 2    asm_pop2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2    asm_cmp_reg   comp_instr \ cmp x1, x2
  1 swap asm_cset      comp_instr \ cset x1, <cond>
  1      asm_push1     comp_instr \ str x1, [x27], 8
;

: asm_comp_cset_zero ( C: cond -- ) ( E: i1 i2 -- bool )
  1      asm_pop1     comp_instr \ ldr x1, [x27, -8]!
  1      asm_cmp_zero comp_instr \ cmp x1, 0
  1 swap asm_cset     comp_instr \ cset x1, <cond>
  1      asm_push1    comp_instr \ str x1, [x27], 8
;

\ ## Numeric comparison

\ https://gforth.org/manual/Numeric-comparison.html
: =   ( i1 i2 -- bool ) [ ASM_EQ asm_comp_cset_reg  ] ;
: <>  ( i1 i2 -- bool ) [ ASM_NE asm_comp_cset_reg  ] ;
: >   ( i1 i2 -- bool ) [ ASM_GT asm_comp_cset_reg  ] ;
: <   ( i1 i2 -- bool ) [ ASM_LT asm_comp_cset_reg  ] ;
: <=  ( i1 i2 -- bool ) [ ASM_LE asm_comp_cset_reg  ] ;
: >=  ( i1 i2 -- bool ) [ ASM_GE asm_comp_cset_reg  ] ;
: <0  ( num   -- bool ) [ ASM_LT asm_comp_cset_zero ] ; \ Or `MI`.
: >0  ( num   -- bool ) [ ASM_GT asm_comp_cset_zero ] ;
: <=0 ( num   -- bool ) [ ASM_LE asm_comp_cset_zero ] ;
: >=0 ( num   -- bool ) [ ASM_GE asm_comp_cset_zero ] ; \ Or `PL`.

: odd ( i1 -- bool ) 1 and ;

\ ## Memory load / store

: @ ( adr -- val ) [
        asm_pop_x1   comp_instr \ ldr x1, [x27, -8]!
  1 1 0 asm_load_off comp_instr \ ldur x1, [x1]
        asm_push_x1  comp_instr \ str x1, [x27], 8
] ;

: ! ( val adr -- ) [
        asm_pop_x1_x2 comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_off comp_instr \ stur x1, [x2]
] ;

: @2 ( adr -- val0 val1 ) [
          asm_pop_x1        comp_instr \ ldr x1, [x27, -8]!
  1 2 1 0 asm_load_pair_off comp_instr \ ldp x1, x2, [x1]
          asm_push_x1_x2    comp_instr \ stp x1, x2, [x27], 16
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

: off! ( adr -- ) 0 swap ! ;
: on!  ( adr -- ) 1 swap ! ;

\ ## Stack introspection and manipulation

\ Floor of data stack.
\ str x26, [x27], 8
: sp0 ( -- adr ) [ ASM_REG_DAT_SP_FLOOR asm_push1 comp_instr ] ;

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
  ASM_REG_DAT_SP ASM_REG_DAT_SP 0 asm_store_off comp_instr \ stur x27, [x27]
  ASM_REG_DAT_SP ASM_REG_DAT_SP 8 asm_add_imm   comp_instr \ add x27, x27, 8
] ;

\ Sets the stack pointer register to the given address.
\ Uses two instructions for the same reason as the above.
: sp! ( adr -- ) [
                   asm_pop_x1  comp_instr \ ldr x1, [x27, -8]!
  ASM_REG_DAT_SP 1 asm_mov_reg comp_instr \ mov x27, x1
] ;

\ Stack introspection doesn't need to be optimal.
: sp_at         ( ind    -- ptr ) cells sp0 + ;
: stack_len     (        -- len ) sp sp0 - /cells ;
: stack_clear   ( …      -- )     sp0 sp! ;
: stack_set_len ( … len  -- … )   sp_at sp! ;
: stack+        ( … diff -- … )   dec cells sp swap + sp! ;
: stack-        ( … diff -- … )   inc cells sp swap - sp! ;

\ ## Characters and strings

: c@ ( str -- char ) [
        asm_pop_x1        comp_instr \ ldr x1, [x27, -8]!
  1 1 0 asm_load_byte_off comp_instr \ ldrb x1, [x1]
        asm_push_x1       comp_instr \ str x1, [x27], 8
] ;

: c! ( char adr -- ) [
        asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 0 asm_store_byte_off comp_instr \ strb x1, [x2]
] ;

: parse_char ( "str" -- char ) parse_word drop c@ ;

:  char' ( E: "str" -- char )           parse_char ;
:: char' ( C: "str" -- ) ( E: -- char ) parse_char comp_push ;

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

0 let: nil
0 let: false
1 let: true

\ Similar to standard `variable`.
: var: ( C: init "name" -- ) ( E: -- adr )
  #word_beg
  sp cell -                  \ Address of the `init` value.
  cell        alloc_data     ( -- adr )
  1           comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr     \ str x1, [x27], 8
  drop                       \ Don't need the `init` value anymore.
  #word_end
;

\ Similar to the standard idiom `create <name> N allot`.
\ Creates a global variable which refers to a buffer of
\ at least the given capacity in bytes.
: buf: ( C: cap "name" -- ) ( E: -- adr cap )
  #word_beg
  dup nil swap alloc_data     ( -- adr )
  1            comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1  comp_instr     \ str x1, [x27], 8
               comp_push      \ str <cap>, [x27], 8
  #word_end
;

\ Shortcut for the standard idiom `create <name> N allot`.
\ Like `buf:` but doesn't return capacity.
: mem: ( C: cap "name" -- ) ( E: -- adr )
  #word_beg
  nil swap    alloc_data     ( -- adr )
  1           comp_page_addr \ `adrp x1, <page>` & `add x1, x1, <pageoff>`
  asm_push_x1 comp_instr     \ str x1, [x27], 8
  #word_end
;

\ Shortcut for the standard idiom `create <name> N cells allot`.
: cells: ( C: len "name" -- ) ( E: -- adr ) cells execute' mem: ;

\ ## Locals: basic
\
\ When we ask for a local, the compiler returns an FP offset.
\ We can compile load/store and push/pop as we like.

: asm_local_get ( reg fp_off -- instr ) ASM_REG_FP swap asm_load_scaled_off ;
: asm_local_set ( reg fp_off -- instr ) ASM_REG_FP swap asm_store_scaled_off ;

\ SYNC[asm_local_read].
: comp_local_get_push ( C: fp_off -- ) ( E: -- val )
  1 swap asm_local_get comp_instr \ ldr x1, [FP, <loc>]
  1      asm_push1     comp_instr \ str x1, [x27], 8
;

\ SYNC[asm_local_write].
: comp_pop_local_set ( C: fp_off -- ) ( E: val -- )
  1      asm_pop1      comp_instr \ ldr x1, [x27, -8]!
  1 swap asm_local_set comp_instr \ str x1, [FP, <loc>]
;

:: to: ( C: "name" -- ) ( E: val -- )
  parse_word comp_named_local comp_pop_local_set
;

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
3 0 extern: strncpy ( dst src str_len -- )
3 0 extern: strlcpy ( dst src buf_len -- )

: move    ( src dst len -- ) swap_over strncpy ;
: fill    ( buf len char -- ) swap memset ;
: erase   ( buf len -- ) 0 fill ;
: blank   ( buf len -- ) 32 fill ;
: compare ( str0 len0 str1 len1 -- direction ) rot min strncmp ;
: str=    ( str0 len0 str1 len1 -- bool ) compare =0 ;
: str<    ( str0 len0 str1 len1 -- bool ) compare <0 ;
: str<>   ( str0 len0 str1 len1 -- bool ) compare <>0 ;

\ ## Exceptions — basic
\
\ Exception definitions are split. See additional words below
\ which support message formatting via the C "printf" family.
\
\ We dedicate one special register to an error value, which
\ is either zero or an address of a null-delimited string.
\ To "throw", we have to store a string into that register
\ and instruct the compiler to insert a "try" check.
\
\ Under the stack-based calling convention, the exception
\ register is `x0`, matching the convention in our C code.

\ The annotation `throws` makes the compiler insert an error check
\ after any call to this procedure, and makes it "contagious": all
\ callers automatically receive "throws".
: throw ( cstr -- ) [
  throws
  0 asm_pop1 comp_instr \ ldr x0, [x27, -8]!
] ;

\ Usage:
\
\   throw" some_error_msg"
\
\ Also see `sthrowf"` for error message formatting.
:: throw" ( C: "str" -- ) comp_cstr compile' throw ;

\ ## Conditionals
\
\ General idea:
\
\   if        \ cbz <outside>
\     if_true
\   end
\   outside
\
\   if        \ cbz <if_false>
\     if_true
\   else      \ b <outside>
\     if_false
\   end
\   outside
\
\ Unlike other Forth systems, we also support chains
\ of "elif" terminated with a single "end".

: reserve_instr ASM_PLACEHOLDER comp_instr ;
: reserve_cond ( -- adr )     asm_pop_x1 comp_instr here reserve_instr ;
: off_to_pc    ( adr -- off ) here swap - ; \ For forward jumps.
: off_from_pc  ( adr -- off ) here      - ; \ For backward jumps.
: patch_branch_forward ( adr -- ) dup off_to_pc asm_branch swap !32 ; \ b <off>
: patch_branch_back    ( adr -- ) off_from_pc   asm_branch swap !32 ; \ b <off>

0 var: COND_HAS

\ cbz x1, <else|end>
: if_patch ( adr_if -- ) dup off_to_pc 1 swap asm_cmp_branch_zero swap !32 ;

\ cbnz x1, <else|end>
: ifn_patch ( adr_ifn -- ) dup off_to_pc 1 swap asm_cmp_branch_not_zero swap !32 ;

: if_done ( cond_prev -- ) COND_HAS ! ;
: if_pop  ( fun_prev adr_if  -- ) if_patch execute ;
: ifn_pop ( fun_prev adr_ifn  -- ) ifn_patch execute ;
: if_init COND_HAS @ COND_HAS on! ' if_done ;

:: elif ( C: -- fun_exec adr_elif fun_elif ) ( E: pred -- )
  ' execute reserve_cond ' if_pop
;

:: elifn ( C: -- fun_exec adr_elif fun_elif ) ( E: pred -- )
  ' execute reserve_cond ' ifn_pop
;

\ With this strange setup, `end` pops the top control frame,
\ which pops the next control frame, and so on. This allows us
\ to pop any amount of control constructs with a single `end`.
\ Prepending `if_done` terminates this chain.
:: if ( C: -- prev fun_done fun_exec adr_if fun_if ) ( E: pred -- )
  if_init execute'' elif
;

:: ifn ( C: -- prev fun_done fun_exec adr_ifn fun_ifn ) ( E: pred -- )
  if_init execute'' elifn
;

: else_pop ( fun_prev adr_else -- ) patch_branch_forward execute ;

:: else
  ( C: fun_exec adr_if fun_if -- adr_else fun_else )
  here to: adr reserve_instr
  ' nop 2 bury \ Disable `fun_exec` to prevent "if" from chaining.
  execute      \ Pop just the top frame.
  adr ' else_pop
;

\ TODO: use raw instruction addresses and `blr`
\ instead of execution tokens and `execute`.
:: end ( C: fun -- ) execute ;

\ ## Loops
\
\ Unlike other Forth systems, we implement termination of arbitrary
\ control structures with a generic `end`. This works for all loops
\ and eliminates the need to remember many different terminators.
\
\ Our solution isn't complicated, either. We simply push procedures
\ with their arguments to the control stack, and pop the latest one
\ with `end`. Each procedure pops its arguments and does something,
\ then pops the next procedure in the chain. The cascade terminates
\ when encountering a procedure which doesn't pop a preceding one.

\ Stack position of a special location in the top loop frame
\ on the control stack. This is where auxiliary structures
\ such as `leave` place their own control frames.
0 var: LOOP_FRAME

: loop_frame! ( frame_ind -- ) LOOP_FRAME ! ;

: loop_frame_init ( -- frame_ind fun )
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
  ifn
    throw" auxiliary loop constructs require an ancestor loop frame"
  end

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

: loop_end ( adr_beg -- ) off_from_pc asm_branch comp_instr ; \ b <begin>
: loop_pop ( fun_prev …aux… adr_beg -- ) loop_end execute'' end ;

\ Implementation note. Each loop frame is "split" in two sub-frames:
\ the "prev frame" and the "current frame". Auxiliary loop constructs
\ such as "leave" and "while" insert their own frames in-between these
\ subframes. See `loop_aux`.
:: loop
  ( C: -- ind_frame fun_frame … adr_beg fun[loop] )
  \ Control frame is split: ↑ auxiliary constructs like "leave" go here.
  loop_frame_init here ' loop_pop
;

\ Breaks out of any loop.
:: leave
  ( C: prev… <loop> …rest -- prev… adr[leave] fun[leave] <loop> …rest )
  stack_len to: len
  here reserve_instr ' else_pop len loop_aux
;

\ Breaks out of any loop.
:: while
  ( C: prev… <loop> …rest -- prev… adr[while] fun[while] <loop> …rest )
  stack_len to: len
  reserve_cond ' if_pop len loop_aux
;

\ Assumes that the top control frame is from `loop`.
:: until ( C: prev… <loop> -- )
      asm_pop_x1          comp_instr \ ldr x1, [x27, -8]!
  1 8 asm_cmp_branch_zero comp_instr \ cbnz x1, 8
  execute
;

: count_loop_pop ( fun_prev …aux… adr_cond adr_beg cond -- )
  to: cond
  loop_end    \ b <beg>
  to: adr
  adr off_to_pc cond asm_branch_cond adr !32 \ b.<cond> <end>
  execute \ Pop auxiliaries; restore the previous loop frame.
;

: for_countdown_loop_pop ( fun_prev …aux… adr_cond adr_beg )
  ASM_LE count_loop_pop
;

: for_countdown_loop_init
  ( C: cur_loc -- ind_frame fun_frame … adr_cond adr_beg fun_for )
  ( E: ceil -- )

  to: cur                \ We'll reuse ceiling as index.
  loop_frame_init
  cur comp_pop_local_set \ Grab the initial ceiling value.

  here to: adr_beg
  1 cur asm_local_get comp_instr    \ ldur x1, [FP, <cur>]
  1     asm_cmp_zero  comp_instr    \ cmp x1, 0
  here to: adr_cond   reserve_instr \ b.<cond> <end>
  1 1 1 asm_sub_imm   comp_instr    \ sub x1, x1, 1
  1 cur asm_local_set comp_instr    \ stur x1, [FP, <cur>]

  adr_cond adr_beg ' for_countdown_loop_pop
;

\ A count-down-by-one loop. Analogue of the non-standard
\ `for … next` loop. Usage; ceiling must be positive:
\
\   123
\   for
\     log" looping"
\   end
\
\ Anton Ertl circa 1994:
\
\ > This is the preferred loop of native code compiler writers
\   who are too lazy to optimize `?do` loops properly.
:: for
  ( C: -- ind_frame fun_frame … adr_cond adr_beg fun_pop )
  ( E: ceil -- )
  comp_anon_local for_countdown_loop_init
;

\ Like `for` but requires a local name to make the index
\ accessible. Usage; ceiling must be positive:
\
\   123
\   -for: ind
\     ind .
\   end
:: -for:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg fun_pop )
  ( E: ceil -- )
  parse_word comp_named_local for_countdown_loop_init
;

: for_countup_loop_pop ( fun_prev …aux… adr_cond adr_beg loc_cur -- )
  to: cur
  1 cur asm_local_get comp_instr \ ldur x1, [FP, <loc>]
  1 1 1 asm_add_imm   comp_instr \ add x1, x1, 1
  1 cur asm_local_set comp_instr \ stur x1, [FP, <loc>]
  ASM_GE count_loop_pop          \ b <begin>; retropatch b.<cond> <end>
;

\ Not used by `for` and `-for:` because they get away
\ with storing just one local instead of two.
\
\ The meaning of "cursor" and "limit" may differ between loop words.
\ The cursor is often a pointer.
: comp_count_loop_init ( loc_cur loc_lim -- adr_cond adr_beg loc_cur )
  to: lim
  to: cur

  here to: adr_beg
  1 cur asm_local_get comp_instr \ ldr x1, [FP, <loc>]
  2 lim asm_local_get comp_instr \ ldr x2, [FP, <loc>]
  1 2   asm_cmp_reg   comp_instr \ cmp x1, x2

  here to: adr_cond
  reserve_instr \ b.<cond> <end>
  adr_cond adr_beg cur
;

\ A count-up-by-one loop. Analogue of the standard `?do … loop` idiom.
\ Requires a name to make the index accessible. Usage:
\
\   123 34
\   +for: ind
\     ind .
\   end
:: +for:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg loc_cur fun_pop )
  ( E: ceil floor -- )
  loop_frame_init

  comp_anon_local             to: lim \ ceil  = limit
  parse_word comp_named_local to: cur \ floor = cursor

  cur     comp_pop_local_set
  lim     comp_pop_local_set
  cur lim comp_count_loop_init
  ' for_countup_loop_pop
;

: loop_countup_pop ( fun_prev …aux… adr_cond adr_beg loc_cur loc_step )
  to: step to: cur

  1 cur  asm_local_get comp_instr \ ldr x1, [FP, <loc>]
  2 step asm_local_get comp_instr \ ldr x2, [FP, <loc>]
  1 1 2  asm_add_reg   comp_instr \ add x1, x1, x2
  1 cur  asm_local_set comp_instr \ str x1, [FP, <off>]
  ASM_GE count_loop_pop           \ b <begin>; retropatch b.<cond> <end>
;

\ Similar to the standard `?do ... +loop`, but terminated with an `end`
\ like other loops. Takes a name to make the index accessible. Usage:
\
\   ceil floor step +loop: ind ind . end
\   123  23    3    +loop: ind ind . end
:: +loop:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg loc_cur loc_step fun_pop )
  ( E: ceil floor step -- )
  loop_frame_init

  comp_anon_local             to: lim \ ceil  = limit
  parse_word comp_named_local to: cur \ floor = cursor
  comp_anon_local             to: step

  step    comp_pop_local_set
  cur     comp_pop_local_set
  lim     comp_pop_local_set
  cur lim comp_count_loop_init
  step ' loop_countup_pop
;

: loop_countdown_pop ASM_LT count_loop_pop ;

\ Like in `+loop:`, the range is `[floor,ceil)`.
:: -loop:
  ( C: "name" -- ind_frame fun_frame … adr_cond adr_beg fun_pop )
  ( E: ceil floor step -- )
  loop_frame_init

  parse_word comp_named_local to: cur \ ceil  = cursor
  comp_anon_local             to: lim \ floor = limit
  comp_anon_local             to: step

  step comp_pop_local_set
  lim  comp_pop_local_set
  cur  comp_pop_local_set

  here to: adr_beg

  \ We can almost use `comp_count_loop_init` here,
  \ but this loop wants subtraction at the start.
  \ TODO use `ldp` for adjacent offsets.
  1 cur  asm_local_get comp_instr    \ ldur x1, [FP, <ceil_off>]
  2 lim  asm_local_get comp_instr    \ ldur x2, [FP, <floor_off>]
  3 step asm_local_get comp_instr    \ ldur x3, [FP, <step_off>]
  1 1 3  asm_sub_reg   comp_instr    \ sub x1, x1, x3
  1 2    asm_cmp_reg   comp_instr    \ cmp x1, x2
  here to: adr_cond    reserve_instr \ b.lt <end>
  1 cur  asm_local_set comp_instr    \ stur x1, [FP, <ceil_off>]

  adr_cond adr_beg ' loop_countdown_pop
;

: ?dup ( val bool -- val ?val ) if dup end ;

\ Finds an extern symbol by name and creates a new word which loads the address
\ of that symbol. The extern symbol can be either a variable or a procedure.
: extern_ptr: ( C: "ours" "extern" -- ) ( E: -- ptr )
  #word_beg
  parse_word extern_got to: adr \ Addr of GOT entry holding extern addr.
  adr 1       comp_page_load    \ `adrp x1, <page>` & `ldr x1, x1, <pageoff>`
  asm_push_x1 comp_instr        \ str x1, [x27], 8
  #word_end
;

\ Like `extern_ptr:` but with an additional load instruction.
\ Analogous to extern vars in C. Lets us access stdio files.
: extern_val: ( C: "ours" "extern" -- ) ( E: -- val )
  #word_beg
  parse_word extern_got to: adr     \ Addr of GOT entry holding extern addr.
  adr 1              comp_page_load \ `adrp x1, <page>` & `ldr x1, x1, <pageoff>`
  1 1 0 asm_load_off comp_instr     \ ldur x1, [x1]
        asm_push_x1  comp_instr     \ str x1, [x27], 8
  #word_end
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

: emit                    putchar ;
: eputchar ( char -- )    stderr fputc ;
: puts     ( cstr -- )    stdout fputs ;
: eputs    ( cstr -- )    stderr fputs ;
: flush                   stdout fflush ;
: eflush                  stderr fflush ;
: type     ( str len -- ) 1 swap stdout fwrite ;
: etype    ( str len -- ) 1 swap stderr fwrite ;
: lf                      10 putchar ; \ Renamed from `cr` which would be a misnomer.
: elf                     10 eputchar ;
: space                   32 putchar ;
: espace                  32 eputchar ;

:  log" parse_str drop puts ;
:: log" comp_cstr compile' puts ;

:  elog" parse_str drop eputs ;
:: elog" comp_cstr compile' eputs ;

\ ## Locals: braces

\ Compiles movement of values from the data stack
\ to the locals in the intuitive "reverse" order.
: } ( locals… len -- ) for comp_pop_local_set end ;

\ Support for the `{ inp0 inp1 -- out0 out1 }` locals notation.
\ Capture order matches stack push order in the parens notation
\ like in Gforth and VfxForth. Types are not supported.
:: {
  0 to: loc_len

  loop
    parse_word to: len to: str
    " }" str len str=
    if loc_len } ret end

    " --" str len str<> while \ `--` to `}` is a comment.

    \ Resulting local token is used by `}` which we're about to call.
    str len comp_named_local ( -- loc_tok )

    loc_len inc to: loc_len
  end

  \ After `--`: skip all words which aren't `}`.
  loop parse_word " }" str<> until

  loc_len }
;

\ For compiling words which modify a local by applying the given function.
: mut_local ( C: fun "name" -- ) ( E: -- )
  parse_word comp_named_local
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
  num two >
  or =0
;

\ ## Varargs and formatting

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
  1 1 ASM_REG_SP -16 asm_store_pair_pre comp_instr \ stp x1, x1, [sp, -16]!
] ;

: pair>systack ( Forth: i1 i2 -- | System: -- i1 i2 ) [
  inline
                     asm_pop_x1_x2      comp_instr \ ldp x1, x2, [x27, -16]!
  1 2 ASM_REG_SP -16 asm_store_pair_pre comp_instr \ stp x1, x2, [sp, -16]!
] ;

\ Note: `add` is one of the few instructions which treat `x31` as `sp`
\ rather than `xzr`. `add x1, sp, 0` disassembles as `mov x1, sp`.
: systack_ptr ( -- sp ) [
  inline
  1 ASM_REG_SP 0 asm_add_imm comp_instr \ add x1, sp, 0 | mov x1, sp
                 asm_push_x1 comp_instr \ str x1, [x27], 8
] ;

\ For debugging.
: sysstack_frame_ptr ( -- fp ) [
  inline
  1 ASM_REG_FP asm_mov_reg comp_instr \ mov x1, x29
               asm_push_x1 comp_instr \ str x1, [x27], 8
] ;

: asm_comp_systack_push ( len -- )
  dup <=0 if drop ret end

  dup odd if
    ' dup>systack inline_word
    dec
  end

  dup ifn drop ret end

  loop
    dup while
    ' pair>systack inline_word
    2 -
  end
  drop
;

: asm_comp_systack_pop ( len -- )
  dup <=0 if ret end
  cells 16 align_up
  \ add sp, sp, <size>
  ASM_REG_SP ASM_REG_SP rot asm_add_imm comp_instr
;

\ Short for "varargs". Sets up arguments for a variadic call.
\ Assumes the Apple Arm64 ABI where varargs use the systack.
\ Usage example:
\
\   : some_word
\     10 20 30 [ 3 ] va{ c" numbers: %zd %zd %zd" printf }va lf
\   ;
\
\ Caution: varargs can only be used in direct calls to variadic procedures.
\ Indirect calls DO NOT WORK because the stack pointer is changed by calls.
:: va{ ( C: N -- N ) ( E: <systack_push> ) dup asm_comp_systack_push ;
:: }va ( C: N -- )   ( E: <systack_pop )       asm_comp_systack_pop  ;

\ Short for "compile variadic args".
: comp_va{ ( C: N -- N ) ( E: <systack_push> ) dup asm_comp_systack_push ;
: }va_comp ( C: N -- )   ( E: <systack_pop> )      asm_comp_systack_pop ;

\ Format-prints to stdout using `printf`. `N` is the variadic arg count,
\ which must be available at compile time. Usage example:
\
\   10 20 30 [ 3 ] logf" numbers: %zd %zd %zd" lf
\
\ TODO define a `:` variant.
:: logf" ( C: N -- ) ( E: i1 … iN -- )
  comp_va{ comp_cstr compile' printf }va_comp
;

\ Format-prints to stderr. TODO define a `:` variant.
:: elogf" ( C: N -- ) ( E: i1 … iN -- )
  comp_va{ compile' stderr comp_cstr compile' fprintf }va_comp
;

\ Formats into the provided buffer using `snprintf`. Usage example:
\
\   SOME_BUF 10 20 30 [ 3 ] sf" numbers: %zd %zd %zd" lf
:: sf" ( C: N -- ) ( E: buf size i1 … iN -- )
  comp_va{ comp_cstr compile' snprintf }va_comp
;

\ ## Exceptions — continued

4096 buf: ERR_BUF

\ Like `sthrowf"` but easier to use. Example:
\
\   10 20 30 [ 3 ] throwf" error codes: %zd %zd %zd"
:: throwf" ( C: len -- ) ( E: i1 … iN -- )
  comp_va{
    compile'  ERR_BUF
    comp_cstr
    compile'  snprintf
  }va_comp
  compile' ERR_BUF
  compile' drop
  compile' throw
;

\ Formats an error message into the provided buffer using `snprintf`,
\ then throws the buffer as the error value. The buffer must be zero
\ terminated; `buf:` ensures this automatically. Usage example:
\
\   4096 buf: SOME_BUF
\   SOME_BUF 10 20 30 [ 3 ] sthrowf" error codes: %zd %zd %zd"
\
\ Also see `throwf"` which comes with its own buffer.
:: sthrowf" ( C: len -- ) ( E: buf size i1 … iN -- )
  comp_va{
    compile'  over
    compile'  swap
    comp_cstr
    compile'  snprintf
  }va_comp
  compile'  throw
;

\ Similar to standard `abort"`, with clearer naming.
:: throw_if" ( C: "str" -- ) ( E: pred -- )
  execute'' if
  comp_cstr
  compile' throw
  execute'' end
;

\ ## Stack printing

: log_int  ( num     -- ) [ 1 ] logf" %zd"  ;
: log_cell ( num ind -- ) dup_over [ 3 ] logf" %zd 0x%zx <%zd>" ;
: .        ( num     -- ) stack_len dec log_cell lf ;

: .s
  stack_len to: len

  len ifn
    log" stack is empty" lf
    ret
  end

  len <0 if
    log" stack length is negative: " len log_int lf
    ret
  end

  len [ 1 ] logf" stack <%zd>:" lf

  len 0 +for: ind
    space space
    ind pick0 ind log_cell lf
  end
;

: .sc .s stack_clear ;

\ ## More memory stuff

extern_val: errno __error
1 1 extern: strerror

6 1 extern: mmap     ( adr len pflag mflag fd off -- adr )
3 1 extern: mprotect ( adr len pflag -- err )

16384 let: PAGE_SIZE
0     let: PROT_NONE
1     let: PROT_READ
2     let: PROT_WRITE
2     let: MAP_PRIVATE
4096  let: MAP_ANON

: mem_map_err ( -- )
  errno dup strerror
  [ 2 ] throwf" unable to map memory; code: %zd; message: %s"
;

: mem_map { size pflag -- addr }
  MAP_ANON MAP_PRIVATE or to: mflag
  0 size pflag mflag -1 0 mmap
  dup -1 = if drop mem_map_err end
;

: mem_unprot_err ( -- )
  errno dup strerror
  [ 2 ] throwf" unable to unprotect memory; code: %zd; message: %s"
;

: mem_unprot ( addr size -- )
  PROT_READ PROT_WRITE or mprotect
  -1 = if throw" unable to mprotect" end
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

\ Allocates space for N cells with lower and upper guards. Underflowing and
\ overflowing into the guards triggers a segfault. The size gets page-aligned,
\ usually leaving additional unguarded readable and writable space between the
\ requested region and the upper guard. The lower guard is immediately below
\ the returned address and immediately prevents underflows.
\
\ TODO consider compiling with lazy-init; dig up the old code.
: cells_guard: ( C: len "name" -- ) ( E: -- addr )
  #word_beg
  cells mem_alloc drop
  1           comp_load  \ ldr x1, <addr>
  1 asm_push1 comp_instr \ str x2, [x27], 8
  #word_end
;
