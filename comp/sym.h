#pragma once
#include "./arch.h"
#include "./lib/bits.h"
#include "./lib/dict.h"
#include "./lib/set.h"
#include "./lib/stack.h"
#include "./lib/str.h"

typedef struct Sym           Sym;
typedef set_of(struct Sym *) Sym_set;

// SYNC[wordlist_enum].
typedef enum : U8 {
  WORDLIST_EXEC = 1,
  WORDLIST_COMP = 2,
} Wordlist;

/*
Metadata for a word in a Forth dictionary / wordset.
- "Norm" words are defined in Forth code.
- "Intrin" words are provided by the interpreter / compiler.
- "Extern" words are dynamically located in linked libraries.

The struct definition is also partially hardcoded
in some Forth examples, and must be kept in sync.

SYNC[sym_fields].
*/
typedef struct Sym {
  enum { SYM_NORM = 1, SYM_INTRIN, SYM_EXTERN } type;

  Word_str name;
  Wordlist wordlist;

  // Every member of the union should begin with an instruction address.
  // This makes it easier to get at them in Forth without an intrinsic.
  union {
    struct {
      Instr     *exec;       // First executable instruction.
      Sym_instrs spans;      // Instruction ranges; used by the assembler.
      bool       inlinable;  // Inner code is safe to copy-paste.
      bool       has_loads;  // Has PC-relative data access.
      bool       has_alloca; // Has dynamic stack allocation, modifies SP.
    } norm;

    void *intrin; // Interpreter intrinsic; implies `interp_only`.
    void *exter;  // Pointer to extern procedure; obtained from `dlsym`.
  };

  Sym_set callees;     // Dependencies in compiled code.
  Sym_set callers;     // Dependents in compiled code.
  U8      inp_len;     // Input parameter count.
  U8      out_len;     // Output parameter count.
  Bits    clobber;     // Clobbers these registers; must include inps and outs.
  bool    throws;      // Requires exception handling in callers.
  bool    catches;     // Automatically catches exceptions from callees.
  bool    comp_only;   // Can only be used between `:` and `;`.
  bool    interp_only; // Forbidden in AOT executables.
} Sym;

typedef stack_of(Sym)  Sym_stack;
typedef dict_of(Sym *) Sym_dict;
