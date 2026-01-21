: \ [ immediate ] 10 parse { -- } ;
: ( [ immediate ] 41 parse { -- } ;

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

\ TODO: consider supporting inference of out-params.
: ASM_INSTR_SIZE  { -- size  } 4      ;
: ASM_REG_DAT_SP  { -- reg   } 27     ;
: ASM_REG_FP      { -- reg   } 29     ;
: ASM_REG_SP      { -- reg   } 31     ;
: ASM_EQ          { -- cond  } 0b0000 ;
: ASM_NE          { -- cond  } 0b0001 ;
: ASM_CS          { -- cond  } 0b0010 ;
: ASM_CC          { -- cond  } 0b0011 ;
: ASM_MI          { -- cond  } 0b0100 ;
: ASM_PL          { -- cond  } 0b0101 ;
: ASM_VS          { -- cond  } 0b0110 ;
: ASM_VC          { -- cond  } 0b0111 ;
: ASM_HI          { -- cond  } 0b1000 ;
: ASM_LS          { -- cond  } 0b1001 ;
: ASM_GE          { -- cond  } 0b1010 ;
: ASM_LT          { -- cond  } 0b1011 ;
: ASM_GT          { -- cond  } 0b1100 ;
: ASM_LE          { -- cond  } 0b1101 ;
: ASM_AL          { -- cond  } 0b1110 ;
: ASM_NV          { -- cond  } 0b1111 ;
: ASM_PLACEHOLDER { -- instr } 666    ; \ udf 666; used in retropatching.

: or { i1 i2 -- i3 }
  \ orr x0, x0, x1
  [ 0b1_01_01010_00_0_00001_000000_00000_00000 comp_instr ]
  \ x0
  i1
;

: and { i1 i2 -- i3 } [
  0b1_00_01010_00_0_00001_000000_00000_00000 comp_instr \ and x0, x0, x1
] i1 ;

: xor { i1 i2 -- i3 } [
  0b1_10_01010_00_0_00001_000000_00000_00000 comp_instr \ eor x0, x0, x1
] i1 ;

: lsl { i1 bits -- i2 } [
  0b1_0_0_11010110_00001_0010_00_00000_00000 comp_instr \ lsl x0, x0, x1
] i1 ;

: lsr { i1 bits -- i2 } [
  0b1_0_0_11010110_00001_0010_01_00000_00000 comp_instr \ lsr x0, x0, x1
] i1 ;

: invert { i1 -- i2 } [
  0b1_01_01010_00_1_00000_000000_11111_00000 comp_instr \ mvn x0, x0
] i1 ;

\ Our parsing rules prevent `1+` or `+1` from being a word name.
: inc { i1 -- i2 } [
  0b1_0_0_100010_0_000000000001_00000_00000 comp_instr \ add x0, x0, 1
] i1 ;

\ Our parsing rules prevent `1-` or `-1` from being a word name.
: dec { i1 -- i2 } [
  0b1_1_0_100010_0_000000000001_00000_00000 comp_instr \ sub x0, x0, 1
] i1 ;

: low_bits { len -- bits } len 1 lsl dec ;
: bit_trunc { imm len -- imm } len low_bits imm and ;

\ Shared by a lot of load and store instructions. 64-bit only.
: asm_pattern_load_store_pair { Xt1 Xt2 Xn imm7 base_instr -- instr }
  Xt2                    10 lsl { Xt2 }
  Xn                      5 lsl { Xn }
  imm7 3 lsr 7 bit_trunc 15 lsl { imm7 }

  base_instr
  Xt1  or
  Xt2  or
  Xn   or
  imm7 or
;

\ Shared by some integer arithmetic and load/store instructions.
: asm_pattern_arith_imm { Xd Xn imm12 -- instr_mask }
  imm12 12 bit_trunc 10 lsl { imm12 }
  Xn                  5 lsl
  imm12 or
  Xd    or
;

: asm_load_byte_off { Wt Wn imm12 -- instr }
  Wt Wn imm12 asm_pattern_arith_imm
  0b00_11_1_0_0_1_01_000000000000_00000_00000 or
;

: asm_store_byte_off { Wt Wn imm12 -- instr }
  Wt Wn imm12 asm_pattern_arith_imm
  0b00_11_1_0_0_1_00_000000000000_00000_00000 or
;

: c@ { str -- char } [
  0 0 0 asm_load_byte_off comp_instr \ ldrb x0, [x0]
] str ;

: char' ( C: "str" -- ) ( E: -- char )
  parse_word        { buf -- }
  buf c@            { char }
  comp_next_inp_reg { reg }
  reg char comp_load
;

\ : mock [ redefine ] \ { -- char }
\   char' A
\   #debug_ctx
\ ;
\ mock

\ 6 1 extern: mmap
\ 3 1 extern: mprotect

\ : mem_map { size pflag -- addr }
\   4096 2 or { MAP_ANON MAP_PRIVATE }
\   MAP_ANON MAP_PRIVATE or { mflag }
\   0 size pflag mflag -1 0 mmap
\ ;

\ : mem_unprot { addr size -- }
\   1 2 { PROT_READ PROT_WRITE }
\   PROT_READ PROT_WRITE or mprotect
\ ;

\ : mem_alloc ( size1 -- addr size2 )
\   0 16384 { PROT_NONE PAGE_SIZE }

\         PAGE_SIZE align_up { size2 }
\   size2 PAGE_SIZE 1 lsl +  { size }
\   size  PROT_NONE mem_map  { addr }
\   addr  PAGE_SIZE +        { addr }
\   addr  size2 mem_unprot
\   addr  size2
\ ;

\ : var: { val -- } ( C: "name" -- ) ( E: -- addr )
\   #word_beg 8 0 comp_static #word_end !
\ ;

\ \ From this point, allocate any amount of control stacks.
\ \ Define operations on them, etc.
\ \ The compiler doesn't need to provide any of that.
\ \
\ \ Define floating point stuff too, while at it.

debug_stack
