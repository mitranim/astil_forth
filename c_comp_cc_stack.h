#pragma once
#include "./c_sym.h"
#include "./lib/dict.h"
#include "./lib/list.h"
#include "./lib/str.h"

typedef struct {
  Word_str name; // May be empty.
} Local;

typedef stack_of(Local)  Local_stack;
typedef dict_of(Local *) Local_dict;

/*
Book-keeping for instructions which we can't properly encode
on the fly when interpreting a colon definition. We reserve
space in form of invalid instructions, and rewrite them when
finalizing a word.

Procedure prologue also implicitly undergoes fixup.
*/
typedef struct {
  enum {
    COMP_FIX_RET = 1,
    COMP_FIX_TRY,
    COMP_FIX_IMM,
    COMP_FIX_RECUR,
  } type;

  union {
    Instr *ret;   // b <epilogue>
    Instr *try;   // cbnz <epilogue>
    Instr *recur; // b <begin>

    struct {
      Instr *instr; // Instruction to patch.
      Uint   num;   // Immediate value to load.
      U8     reg;   // Register to load into.
    } imm;
  };
} Comp_fixup;

typedef list_of(Comp_fixup) Comp_fixups;

// Transient context used in compilation of a single word.
typedef struct {
  Sym        *sym;
  Comp_fixups fixup;
  Local_stack locals;
  Local_dict  local_dict;
  bool        redefining; // Temporarily suppress "redefined" diagnostic.
  bool        compiling;  // Turned on by `:` and `]`, turned off by `[`.
} Comp_ctx;
