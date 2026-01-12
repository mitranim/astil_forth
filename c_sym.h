#pragma once
#include "./c_asm.h"
#include "./lib/dict.h"
#include "./lib/set.h"
#include "./lib/stack.h"
#include "./lib/str.h"

typedef struct Sym           Sym;
typedef set_of(struct Sym *) Sym_set;

/*
Metadata for a word in a Forth dictionary / wordset.
- "Norm" words are defined in Forth code.
- "Intrin" words are provided by the interpreter / compiler.
- "Extern" words are declared in Forth code and located in dylibs.
*/
typedef struct Sym {
  Word_str name;

  enum { SYM_NORM = 1, SYM_INTRIN = 2, SYM_EXT_PTR = 3, SYM_EXT_PROC = 4 } type;

  union {
    struct {
      Sym_instrs spans;     // Created and used by the assembler.
      Instr_span exec;      // Executable code range.
      bool       inlinable; // Manual toggle only.
      bool       has_loads; // Has PC-relative data access.
      bool       has_rets;  // Has explicit early returns.
    } norm;

    struct {
      void *fun; // Interpreter intrinsic; implies `interp_only`.
    } intrin;

    struct {
      void *addr; // Obtained from `dlsym`.
    } ext_ptr;

    struct {
      void *fun; // Obtained from `dlsym`.
      U8    inp_len;
      U8    out_len;
    } ext_proc;
  };

  Sym_set callees;     // Dependencies.
  Sym_set callers;     // Dependents.
  bool    throws;      // Requires error handling.
  bool    immediate;   // Interpret immediately even in compilation mode.
  bool    comp_only;   // Can only be used between `:` and `;`.
  bool    interp_only; // Forbidden in final executables.
} Sym;

typedef stack_of(Sym)  Sym_stack;
typedef dict_of(Sym *) Sym_dict;
