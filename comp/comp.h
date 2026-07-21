#pragma once
#include "../clib/dict.h"
#include "../clib/mem.h"
#include "../clib/num.h"
#include "../clib/str.h"
#include "./arch.h"

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
    ASM_FIX_THROW,
    ASM_FIX_RECUR,
  } type;

  union {
    Instr *ret; // b <epi_ok>

    struct {
      U8     err_reg;
      Instr *instr; // cbnz <err_reg>, <epi_err>
    } try;

    Instr *throw; // b <epi_err>
    Instr *recur; // b <begin>
  };
} Asm_fixup;

typedef stack_of(Asm_fixup) Asm_fixups;

#ifndef CALL_CONV_STACK
#include "./comp_cc_reg.h"
#else
#include "./comp_cc_stack.h"
#endif

// 0x100000 * sizeof(Instr) = 4 MiB
// SYNC[instr_heap_len].
static constexpr Uint INSTR_HEAP_LEN = (1u << 22u) / sizeof(Instr);

// SYNC[main_arena_len].
static constexpr Uint MAIN_ARENA_LEN = 1u << 30u;

/*
Fixed-size code heap. Stupid, simple solution.

Instruction addresses need to be stable because they're referenced in multiple
places in C structures and used in Forth during compilation, by various control
flow words such as conditionals and loops.

TODO: consider writing interp-only words to a separate sector,
allowing us to elide them from the executable.

SYNC[instr_heap_fields].
*/
typedef struct {
  U8    guard_0[MEM_PAGE];      // PROT_NONE
  Instr instrs[INSTR_HEAP_LEN]; // Executable instructions.
  U8    guard_1[MEM_PAGE];      // PROT_NONE
} Instr_heap;

/*
Sections are accessed and modified via `Comp_code`.

Sections `.intrins` and `.externs` are used for dynamic linking,
analogous to a GOT (global offset table) in executable formats.
*/
typedef struct {
  Instr_heap exec;                            // 4 MiB; executable code.
  U8         data[1u << 29u];                 // 512 MiB; mutable values.
  U8         guard_0[MEM_PAGE];               // PROT_NONE
  U64        externs[MEM_PAGE / sizeof(U64)]; // Addresses of external symbols.
  U8         guard_1[MEM_PAGE];               // PROT_NONE
  U64        intrins[MEM_PAGE / sizeof(U64)]; // Addresses of intrinsic procs.
  U8         guard_2[MEM_PAGE];               // PROT_NONE
  U8         arena[MAIN_ARENA_LEN];           // Main arena for ambient context.
  U8         guard_3[MEM_PAGE];               // PROT_NONE
} Comp_heap;

// SYNC[main_arena_off].
static constexpr Uint MAIN_ARENA_OFF = offsetof(Comp_heap, arena);
static_assert(MAIN_ARENA_OFF == 0x2041C000);

// Invariants: `.addrs.len == stack_len(.names) == .inds.len`.
//
// SYNC[comp_syms_fields].
typedef struct {
  U64_list           addrs; // References `Comp_heap.intrins` or `.externs`.
  stack_of(Word_str) names; // Backing storage for keys in `.inds`.
  Ind_dict           inds;  // Indexes in `.vals`; keys come from `.names`.
} Comp_syms;

// SYNC[comp_code_fields].
typedef struct {
  Instr_heap *write;           // Writable non-executable instructions.
  Comp_heap  *heap;            // Executable code and data.
  Instr_list  code_write;      // References `.code.instrs`.
  Instr_list  code_exec;       // References `.heap.code.instrs`.
  Buf         data;            // References `.heap.data`.
  Comp_syms   externs;         // Extern symbols in `.heap.externs`.
  Comp_syms   intrins;         // Intrin symbols in `.heap.intrins`.
  Ind         valid_instr_len; // Further instructions may be unpatched.
} Comp_code;

// SYNC[comp_fields].
typedef struct {
  Comp_code code;
  Comp_ctx  ctx;
} Comp;
