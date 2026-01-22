#pragma once
#include "./c_sym.h"
#include "./lib/dict.h"
#include "./lib/stack.h"
#include "./lib/str.h"

typedef struct {
  Word_str name;   // May be empty.
  Ind      mem;    // Index of FP offset.
  bool     inited; // True if `mem` has been assigned.
} Local;

typedef stack_of(Local)  Local_stack;
typedef dict_of(Local *) Local_dict;

// Transient context used in compilation of a single word.
typedef struct {
  Sym        *sym;
  Asm_fixups  asm_fix;
  Local_stack locals;
  Local_dict  local_dict;
  Ind         mem_locs;   // How many locals need stack memory.
  bool        redefining; // Temporarily suppress "redefined" diagnostic.
  bool        compiling;  // Turned on by `:` and `]`, turned off by `[`.
} Comp_ctx;
