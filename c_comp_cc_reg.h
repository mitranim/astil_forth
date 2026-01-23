#pragma once
#include "./c_arch.h"
#include "./c_sym.h"
#include "./lib/bits.h"
#include "./lib/dict.h"
#include "./lib/num.h"
#include "./lib/str.h"

typedef struct Local_write Local_write;

/*
Book-keeping structure describing a local variable.

Operations on locals, synopsis:
- "get"   -- mov from a known temp register; fall back on a "read"
- "set"   -- reset to given temp register
- "read"  -- mov or load from the stable location; confirm a prior "write"
- "write" -- mov or store from temp register to stable location

When locals are "set" or "got", they're associated with "temp" registers.
The association is invalidated by clobbers, which occur for two reasons:

1. The register is about to be used for something else:
- Input argument to a procedure about to be called.
- Output value from a procedure about to be called.
- Any other clobber in procedure about to be called.

2. We're about to meet a "clobber barrier" which invalidates ALL
temp register associations. These barriers occur at EVERY branch
point in conditionals and loops. A dirty but ultra simple way of
avoiding data flow analysis which requires an IR and obliterates
the dream of simplicity.

Locals always begin in temp registers without a stable location.
Locals used in "read" operations and thus in "write" operations,
eventually get a stable location, which is either an unclobbered
scratch register or an FP offset on the system stack.

"get" and "set" are abstract operations. An intrinsic for "set" is used by
assignment via `to:`; this doesn't always produce a "write". "get" is used
internally whenever a local is mentioned by name; sometimes it produces no
instructions, sometimes a `mov` from a known temp register. Failing that,
"get" falls back on a real "read", which requires a real prior "write",
which we insert in advance.

"read" and "write" are data-copy operations. Depending on the eventual
location of the local, they become either `mov` between registers, or
`ldr` and `str` from/to stack memory. These operations always involve
a known temp register, which is either a source or a destination.
A "read" always associates the local with that temp reg in addition
to the local's stable location and other temp regs.

"write" is emitted lazily on clobbers, and is initially unconfirmed.
We still reserve an instruction to be patched; an unconfirmed write
is later replaced with a `nop`; obviously suboptimal but avoids the
need for IR and multiple passes. The next "read" operation for this
local confirms every preceding unconfirmed "write". Every confirmed
"write" is later rewritten with a `mov` or `str` instruction.

When a local runs out of temp registers and we emit a "write" operation,
the local is considered "stable" until the next "set". Its location now
holds an up-to-date value, and can be repeatedly "read" from. The local
may now be repeatedly associated and disassociated with temp registers,
without "running out" and creating new "write" operations. However, the
next "set" operation invalidates this state.
*/
typedef struct {
  Word_str     name;
  Local_write *write;  // Latest unconfirmed "write"; confirmed by "reads".
  bool         stable; // Has up-to-date value in assigned stable location.

  // Final stable location used for writes and reads.
  // Locals which are never "read" are not considered.
  enum {
    LOC_UNKNOWN,
    LOC_REG,
    LOC_MEM,
  } location;

  union {
    U8  reg; // For `LOC_REG` only.
    Ind mem; // For `LOC_MEM` only; index of FP offset.
  };
} Local;

typedef stack_of(Local)  Local_stack;
typedef dict_of(Local *) Local_dict;

typedef struct {
  Local *loc;
  Instr *instr; // Retropatched with mov or load.
  U8     reg;   // Reg is immediately known; location is unknown.
} Local_read;

typedef struct Local_write {
  Local              *loc;
  Instr              *instr;     // Retropatched with mov or store.
  struct Local_write *prev;      // Prior "write" for chain-confirming.
  U8                  reg;       // Reg is known; location is unknown.
  bool                confirmed; // If not, turn this into a nop.
} Local_write;

typedef struct {
  enum {
    LOC_FIX_READ = 1,
    LOC_FIX_WRITE,
  } type;

  Local_read  read;
  Local_write write;
} Loc_fixup;

typedef stack_of(Loc_fixup) Loc_fixups;

/*
Transient context used in compilation of a single word.

Internal note on argument handling. Words can be immediate and regular.
Immediate words always communicate through the interpreter's data stack.
When invoking an immediate word, its arguments come from the data stack,
and its outputs go back into the data stack. All of this happens without
any effects on the compile-time context by default. Some immediate words
choose to affect compilation by invoking various compiler intrinsics.
*/
typedef struct {
  Sym        *sym;        // What we're currently compiling.
  Local_stack locals;     // Includes current word's input params.
  Local_dict  local_dict; // So we can find locals by name.
  Ind         mem_locs;   // How many locals need stack memory.
  Local      *loc_regs[ARCH_ALL_PARAM_REG_LEN]; // Temp reg-local associations.
  Bits        vol_regs;   // Volatile registers available for locals.
  U8          arg_low;    // How many args got consumed by assignments.
  U8          arg_len;    // Available args for the next call or assignment.
  Asm_fixups  asm_fix;    // Used for patching instructions in a post-pass.
  Loc_fixups  loc_fix;    // Used for resolving stable locations for locals.
  bool        redefining; // Temporarily suppress "redefined" diagnostic.
  bool        compiling;  // Turned on by `:` and `]`, turned off by `[`.
} Comp_ctx;
