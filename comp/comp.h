#pragma once
#include "./arch.h"
#include "./lib/dict.h"
#include "./lib/mem.h"
#include "./lib/num.h"
#include "./lib/str.h"

/*
Book-keeping for instructions which we can't properly encode
on the fly when interpreting a colon definition. We reserve
space in form of invalid instructions, and rewrite them when
finalizing a word.

TODO more precise name since we also have `Loc_fixup`, which
is used in the register callvention; see `./comp_cc_reg.h`.

Procedure prologue also implicitly undergoes fixup.
*/
typedef struct {
  enum {
    ASM_FIX_RET = 1,
    ASM_FIX_TRY,
    ASM_FIX_RECUR,
    ASM_FIX_IMM,
  } type;

  union {
    Instr *ret;   // b <epilogue>
    Instr *try;   // cbnz <epilogue>
    Instr *recur; // b <begin>

    struct {
      Instr *instr; // Instruction to patch.
      Sint   num;   // Immediate value to load.
      U8     reg;   // Register to load into.
    } imm;
  };
} Asm_fixup;

typedef stack_of(Asm_fixup) Asm_fixups;

#ifdef NATIVE_CALL_CONV
#include "./comp_cc_reg.h"
#else
#include "./comp_cc_stack.h"
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
  U8         data[1 << 18];               // 256 KiB; mutable values.
  U8         guard_0[MEM_PAGE];           // PROT_NONE
  U64        got[MEM_PAGE / sizeof(U64)]; // Global offset table.
  U8         guard_1[MEM_PAGE];           // PROT_NONE
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
  U8_list     data;            // References `.heap.datas`.
  U64_list    got;             // References `.heap.got`.
  Comp_got    gots;            // Offsets of intrin/extern symbols in GOT.
  Ind         valid_instr_len; // Further instructions may be unpatched.
} Comp_code;

typedef struct {
  Comp_code code;
  Comp_ctx  ctx;
} Comp;
