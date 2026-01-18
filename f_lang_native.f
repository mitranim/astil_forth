: \ [ immediate ] 10 parse { -- } ;
: ( [ immediate ] 41 parse { -- } ;

: abort [ 0b110_101_00_001_0000001010011010_000_00 comp_instr ] ;
: unreachable abort ;
: nop ;

: or { i1 i2 -- i3 } [
  0b1_01_01010_00_0_00001_000000_00000_00000 comp_instr \ orr x0, x0, x1
] ;

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

6 1 extern: mmap
3 1 extern: mprotect

: mem_map { size pflag -- addr }
  4096 2 or { MAP_ANON MAP_PRIVATE }
  MAP_ANON MAP_PRIVATE or { mflag }
  0 size pflag mflag -1 0 mmap
;

: mem_unprot { addr size -- }
  1 2 { PROT_READ PROT_WRITE }
  PROT_READ PROT_WRITE or mprotect
;

: mem_alloc ( size1 -- addr size2 )
  0 16384 { PROT_NONE PAGE_SIZE }

        PAGE_SIZE align_up { size2 }
  size2 PAGE_SIZE 1 lsl +  { size }
  size  PROT_NONE mem_map  { addr }
  addr  PAGE_SIZE +        { addr }
  addr  size2 mem_unprot
  addr  size2
;

: var: { val -- } ( C: "name" -- ) ( E: -- addr )
  #word_beg 8 0 comp_static #word_end !
;

\ From this point, allocate any amount of control stacks.
\ Define operations on them, etc.
\ The compiler doesn't need to provide any of that.
\
\ Define floating point stuff too, while at it.
