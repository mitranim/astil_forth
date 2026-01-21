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

Locals evicted to memory are stored at unique FP offsets.
The offset is derived from each local's index in the list.
*/
typedef struct {
  Word_str     name;
  Local_write *write; // Latest unconfirmed "write"; confirmed by "reads".

  // Final stable location used for writes and reads.
  // Locals which are never "read" are not considered.
  enum {
    LOC_UNKNOWN,
    LOC_REG,
    LOC_MEM,
  } location;

  union {
    U8  reg; // For `LOC_REG` only.
    Ind mem; // For `LOC_MEM` only; 1-indexed position of FP offset.
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
    COMP_FIX_LOC_READ,
    COMP_FIX_LOC_WRITE,
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

    Local_read  loc_read;
    Local_write loc_write;
  };
} Comp_fixup;

typedef stack_of(Comp_fixup) Comp_fixups;

/*
Transient context used in compilation of a single word.

Internal notes.

`.inited` controls the mode of `{`. When false, `{` declares parameters.
When true, `{` assigns locals. Set to `true` upon encountering the first
non-immediate word, or declaring parameters with `{`.

Immediate words are called without checking their input-output signatures,
and without any implicit effect on the compilation state. Immediate words
which modify compilation state, such as conditionals, interact with the
compiler imperatively rather than declaratively, by invoking intrinsics.
They also communicate "out of band" through the data stack.
*/
typedef struct {
  Sym        *sym;         // What we're currently compiling.
  Local_stack locals;      // Includes current word's input params.
  Local_dict  local_dict;  // So we can find locals by name.
  Ind         loc_mem_len; // How many locals need stack memory.
  Local      *loc_regs[ASM_ALL_PARAM_REG_LEN]; // Temp reg-local associations.
  Bits        vol_regs;   // Volatile registers available for locals.
  U8          arg_low;    // How many args got consumed by assignments.
  U8          arg_len;    // Available args for the next call or assignment.
  Comp_fixups fixup;      // Used for patching instructions in a post-pass.
  bool        redefining; // Temporarily suppress "redefined" diagnostic.
  bool        compiling;  // Turned on by `:` and `]`, turned off by `[`.
  bool        proc_body;  // Found a non-immediate word or brace params.
} Comp_ctx;
