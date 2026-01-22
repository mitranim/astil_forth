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

: comp_push { val -- } comp_next_arg_reg { reg } val reg comp_load ;
: next_word { -- XT } ( "word" -- XT ) parse_word find_word ;
: word'     { -- XT } next_word ;
: '         ( C: "word" -- ) ( E: -- XT ) next_word comp_push ;
: inline'   ( C: "word" -- ) ( E: word ) next_word inline_word ;
: postpone' ( C: "word" -- ) ( E: word ) next_word comp_call ;
: compile' next_word comp_push ' comp_call comp_call ;

: @ { adr -- val } [
  comp_next_arg
  0b11_111_0_00_01_0_000000000_00_00000_00000 comp_instr \ ldur x0, [x0]
  \ TODO:
  \ 0 0 0 asm_load_off comp_instr \ ldur x0, [x0]
] ;

: ! { val adr -- } [
  0b11_111_0_00_00_0_000000000_00_00001_00000 comp_instr \ stur x0, [x1]
  \ TODO:
  \ 0 1 0 asm_store_off comp_instr \ stur x0, [x1]
] ;

\ For words which define words. Kinda like `create`.
: #word_beg compile' : ;
: #word_end compile' ; not_comp_only ;

\ Similar to standard `constant`.
: let: { val -- } ( C: "name" -- ) ( E: -- val )
  #word_beg
  immediate \ Compiling a compiling word.
  val comp_push compile' comp_push
  #word_end
;

8 let: cell

\ Similar to standard `variable`.
: var: { init -- } ( C: "name" -- ) ( E: -- adr )
  0 cell alloc_data { adr }
  adr postpone' let:
  init adr !
;

4      let: ASM_INSTR_SIZE
27     let: ASM_REG_DAT_SP
29     let: ASM_REG_FP
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
  comp_next_arg_reg { reg }
  char reg comp_load
;

\ Same as standard `s"` in interpretation mode.
\ The parser ensures a trailing null byte.
\ All our strings are zero-terminated.
: parse_str { -- cstr len } char' " parse ;
: parse_cstr { -- cstr } parse_str { cstr -- } cstr ;

\ Same as standard `s"` in interpretation mode.
\ For top-level code in scripts. Inside words, use `"`.
: str" { -- cstr len } parse_str ;
: cstr" { -- cstr } parse_str { str -- } str ;

: comp_str ( C: <str> -- ) ( E: -- cstr len )
  parse_str          { buf len }
  len inc            { cap } \ Reserve terminating null byte.
  buf cap alloc_data { adr }
  comp_next_arg_reg  { RS }
  comp_next_arg_reg  { RL }
  adr RS comp_page_addr \ `adrp RS, <page>` & `add RS, RS, <pageoff>`
  len RL comp_load      \ ldr RL, <len>
;

\ Same as standard `s"` in compilation mode.
\ Words ending with a quote are automatically immediate.
: " ( C: <str> -- ) ( E: -- cstr len ) comp_str ;

: comp_cstr ( C: <str> -- ) ( E: -- cstr )
  parse_str          { buf len }
  len inc            { cap } \ Reserve terminating null byte.
  buf cap alloc_data { adr }
  comp_next_arg_reg  { reg }
  adr reg comp_page_addr \ `adrp <reg>, <page>` & `add <reg>, <reg>, <pageoff>`
;

: c" comp_cstr ;
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
