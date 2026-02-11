#pragma once
#include "./lib/dict.h"
#include "./lib/stack.h"
#include "./lib/str.h"
#include "./sym.h"

// #ifdef CLANGD
// #include "./comp.h"
// #endif

typedef struct {
  Word_str name;   // May be empty.
  Ind      fp_off; // Offset from frame pointer.
  bool     inited; // True if `fp_off` has been assigned.
} Local;

typedef stack_of(Local)  Local_stack;
typedef dict_of(Local *) Local_dict;

// Transient context used in compilation of a single word.
typedef struct {
  Sym        *sym;
  Asm_fixups  asm_fix;
  Local_stack locals;
  Local_dict  local_dict;
  Ind         anon_locs;  // For auto-naming of anonymous locals.
  Ind         fp_off;     // Stack space reserved for locals.
  bool        redefining; // Temporarily suppress "redefined" diagnostic.
  bool        compiling;  // Turned on by `:` and `]`, turned off by `[`.
  bool        has_alloca; // True if SP is dynamically adjusted in the body.
} Comp_ctx;
