#pragma once
#include "./c_arch.h"
#include "./lib/dict.h"
#include "./lib/mem.h"
#include "./lib/num.h"
#include "./lib/str.h"

#ifdef NATIVE_CALL_ABI
#include "./c_comp_native.h"
#else
#include "./c_comp_stack.h"
#endif

// 4 MiB
static constexpr Uint INSTR_HEAP_LEN = (1 << 22) / sizeof(Instr);

/*
Fixed-size code heap. Stupid, simple solution.

Instruction addresses need to be stable because they're referenced in multiple
places in C structures and used in Forth during compilation, by various control
flow words such as conditionals and loops.

TODO: consider writing interp-only words to a separate sector,
allowing us to elide them from the executable.
*/
typedef struct {
  U8    guard_0[MEM_PAGE];      // PROT_NONE
  Instr instrs[INSTR_HEAP_LEN]; // Executable instructions.
  U8    guard_1[MEM_PAGE];      // PROT_NONE
} Instr_heap;

// Sections are accessed and modified via `Comp_code`.
typedef struct {
  Instr_heap exec;                        // 4 MiB; executable code.
  U8         consts[1 << 18];             // 256 KiB; constant values.
  U8         guard_0[MEM_PAGE];           // PROT_NONE
  U8         data[1 << 18];               // 256 KiB; mutable values.
  U8         guard_1[MEM_PAGE];           // PROT_NONE
  U64        got[MEM_PAGE / sizeof(U64)]; // Global offset table.
  U8         guard_2[MEM_PAGE];           // PROT_NONE
} Comp_heap;

// "GOT" = global offset table.
typedef struct {
  stack_of(Word_str) names;
  Ind_dict           inds;
} Comp_got;

typedef struct {
  Instr_heap *write;           // Writable non-executable instructions.
  Comp_heap  *heap;            // Executable code and data.
  Instr_list  code_write;      // References `.code.instrs`.
  Instr_list  code_exec;       // References `.heap.code.instrs`.
  U8_list     consts;          // References `.heap.consts`.
  U8_list     data;            // References `.heap.datas`.
  U64_list    got;             // References `.heap.got`.
  Comp_got    gots;            // Offsets of intrin/extern symbols in GOT.
  Ind         valid_instr_len; // Further instructions may be unpatched.
} Comp_code;

typedef struct {
  Comp_code code;
  Comp_ctx  ctx;
} Comp;
