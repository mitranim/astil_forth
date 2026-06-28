#pragma once
#include "../clib/bits.h"
#include "../clib/dict.h"
#include "../clib/num.h"
#include "../clib/str.h"
#include "./arch.h"
#include "./sym.h"

// #ifdef CLANGD
// #include "./comp.h"
// #endif

typedef struct Loc_reloc Loc_reloc;

/*
Book-keeping structure describing a local variable.

Local lifecycle ALWAYS begins by associating with some argument register.
Mentioning a local by name is a "push from local" operation, which copies
its value into the next argument register, associating it with the local.
When a local runs out of argument registers due to clobbers, the compiler
emits a tentative relocation by reserving an instruction and adding a new
`Loc_reloc` asm fixup entry; the assembler patches this when finalizing a
function, when other clobbers are known.

Local relocations are initially tentative. Subsequently accessing a local
confirms its prior relocs. When possible, unconfirmed relocs are elided,
and when not, the instructions are replaced with nops.

Relocations copy local values to their eventual stable locations,
which are undecided until we finalize the current function. Those
locations can be:
- Argument registers; architecturally this is better-known as caller-saved
  registers, or volatile scratch registers. We use them like a "stack".
- Stable callee-saved registers.
- Memory locations (offsets from the frame pointer).

We have "clobber barriers" which invalidate all argument registers,
evicting all locals to their eventual "stable" locations. Barriers
are used at branch points and in recursion. This simple solution
lets us avoid a complex IR and liveness analysis.

When compiling a function, we track clobbers, and when finalizing,
we allocate one location per local, for the duration of the entire
function, in the priority order listed above: arg > stable > mem.

When "pushing" a local to the compile-time "register stack",
the compiler looks for registers already associated with the
local in the current position, and falls back on a tentative
"read fixup" which will copy the local from its stable place
which is only decided when finalizing a function. Similar to
relocations, this reserves an instruction for later patching.

Internal invariant: a local EITHER has some argument registers,
OR has been relocated. We do not support un-initialized locals
with unknown locations.

Locals have a special lifecycle phase between two events:
- 0: run out of arg registers -> get relocated
- 1: get reassigned

Between these two events, a local is marked as "stable".
It can be repeatedly associated with arg regs via reads,
and repeatedly disassociated due to clobbers without any
new relocations. Reassignment clears this flag, possibly
incurring new relocations.
*/
typedef struct {
  Word_str   name;
  Loc_reloc *reloc;  // Latest unconfirmed "reloc"; confirmed by "reads".
  bool       stable; // Has up-to-date value in assigned stable location.
  bool       read;   // Has a read; auto-confirm all subsequent writes.
  bool       used;   // Has ever been mentioned by name; superset of `read`.

  // Final stable location used for writes and reads.
  // Locals which are never "read" are not considered.
  enum {
    LOC_UNKNOWN,
    LOC_REG,
    LOC_MEM,
  } location;

  union {
    U8  reg;    // For `LOC_REG` only.
    Ind fp_off; // For `LOC_MEM` only; FP offset.
  };
} Local;

typedef stack_of(Local)  Loc_stack;
typedef dict_of(Local *) Loc_dict;

typedef struct {
  Local *loc;
  Instr *instr; // Retropatched with mov or load.
  U8     reg;   // Reg is immediately known; location is unknown.
} Loc_read;

typedef struct Loc_reloc {
  Local            *loc;
  Instr            *instr;     // Retropatched with mov or store.
  struct Loc_reloc *prev;      // Prior "reloc" for chain-confirming.
  U8                reg;       // Reg is known; location is unknown.
  bool              confirmed; // If not, turn this into a nop.
} Loc_reloc;

// SYNC[loc_fixup_fields].
typedef struct {
  // SYNC[loc_fixup_types].
  enum {
    LOC_FIX_READ = 1,
    LOC_FIX_RELOC,
  } type;

  Loc_read  read;
  Loc_reloc reloc;
} Loc_fixup;

typedef stack_of(Loc_fixup) Loc_fixups;

/*
We track comptime constants, and the `[floor,ceil)` ranges
for the instructions generated for placing them into regs.
Some parts of the system, such as our constant folding and
`alloca`, detect and "consume" constants, backtracking the
instructions. Assembler always encodes immediates inline.
*/
typedef struct {
  Sint num;
  Ind  floor; // For backtracking: first instruction.
  Ind  ceil;  // For backtracking: just above last instruction.
} Comp_arg_imm;

typedef struct {
  Ind try_instr_floor;
  Ind try_instr_ceil;
  Ind try_fix_ind;
} Comp_arg_err;

/*
Value associated with a register.

SYNC[comp_arg_fields].
*/
typedef struct {
  enum {
    COMP_ARG_UNKNOWN,
    COMP_ARG_IMM,
    COMP_ARG_LOC,
    COMP_ARG_ERR,
  } type;

  union {
    Comp_arg_imm imm;
    Local       *loc;
    Comp_arg_err err;
  };
} Comp_arg;

/*
Transient context used in compilation of a single word.

Internal note on argument handling. Words can be immediate and regular.
Immediate words always communicate through the interpreter's data stack.
When invoking an immediate word, its arguments come from the data stack,
and its outputs go back into the data stack. All of this happens without
any effects on the compile-time context by default. Some immediate words
choose to affect compilation by invoking various compiler intrinsics.

SYNC[comp_ctx_fields].
*/
typedef struct {
  Sym       *sym;                   // What we're currently compiling.
  Loc_stack  locals;                // Includes current word's input params.
  Loc_dict   local_dict;            // So we can find locals by name.
  Ind        anon_locs;             // For auto-naming of anonymous locals.
  Ind        fp_off;                // Stack space reserved for locals.
  Comp_arg   args[ASM_ARG_LEN_MAX]; // Values in arg registers.
  Bits       vol_regs;              // Volatile registers available for locals.
  U8         saved_reg;  // Lowest callee-saved register used for locals.
  U8         arg_len;    // Available args for the next call or assign.
  Asm_fixups asm_fix;    // For patching instructions in a post-pass.
  Loc_fixups loc_fix;    // For resolving stable locations for locals.
  bool       redefining; // Temporarily suppress "redefined" diagnostic.
  bool       compiling;  // Turned on by `:` and `]`, turned off by `[`.
  bool       has_alloca; // True if SP is dynamically adjusted in the body.
  bool       try_all;    // Auto-"try" when calling words with `.has_err`.
  bool       slop;       // Disable rejection of sloppy code.
} Comp_ctx;
