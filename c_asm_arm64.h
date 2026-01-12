#pragma once
#include "./lib/dict.h"
#include "./lib/list.h"
#include "./lib/num.h"
#include "./lib/stack.h"
#include "./lib/str.h"

typedef U32 Instr;

typedef list_of(Instr) Instr_list;
typedef span_of(Instr) Instr_span;

static_assert(sizeof(Uint) >= sizeof(U32));
static_assert(sizeof(Sint) >= sizeof(S32));
static_assert(sizeof(void *) == sizeof(Uint));
static_assert(sizeof(void *) == sizeof(Sint));

/*
Maximum register-only input/output count allowed by C under arm64 ABI.
Calling procedures with fewer actual inputs or outputs is harmless.
*/
typedef Sint(Extern_fun)(Sint, Sint, Sint, Sint, Sint, Sint, Sint, Sint);

/*
Book-keeping for instructions which we can't correctly encode during the first
pass. During the first pass, we reserve space in form of invalid instructions,
and rewrite them when finalizing a word.

Procedure prologue also implicitly undergoes fixup.
*/
typedef struct {
  enum {
    ASM_FIXUP_RET   = 1,
    ASM_FIXUP_TRY   = 2,
    ASM_FIXUP_RECUR = 3,
    ASM_FIXUP_LOAD  = 4
  } type;

  Instr *ret;   // b <epilogue>
  Instr *try;   // cbnz <epilogue>
  Instr *recur; // b <begin>

  struct {
    Instr *instr; // Instruction to patch.
    Uint   imm;   // Immediate value to load.
    U8     reg;   // Register to load into.
  } load;
} Asm_fixup;

typedef list_of(Asm_fixup) Asm_fixup_list;

/*
Stupid, simple, not scalable.

Reasons for a fixed-size heap:
- Simplicity.
- Stable addresses.

Instruction addresses need to be stable because they're referenced in multiple
places in C structures and used in Forth during compilation, by various control
flow words such as conditionals and loops.

Sections are accessed and modified via lists in `Asm_heap`, with bounds checks.
*/
typedef struct {
  U8    guard_low[1 << 14];                // 16 KiB
  Instr instrs[(1 << 20) / sizeof(Instr)]; // 1 MiB
  U8    consts[1 << 18];                   // 256 KiB
  U8    datas[1 << 18];                    // 256 KiB
  U64   got[(1 << 14) / sizeof(U64)];      // 16 KiB; "global offset table"
  U8    guard_high[1 << 14];               // 16 KiB
} Asm_page;

typedef struct {
  Asm_page  *page;
  Instr_list instrs; // References `.page.instrs`.
  U8_list    consts; // References `.page.consts`.
  U8_list    datas;  // References `.page.datas`.
  U64_list   got;    // References `.page.got`.
} Asm_heap;

typedef struct {
  stack_of(Word_str) names;
  Ind_dict           inds;
} Asm_got;

// Positions of "locals"; the dict owns the keys.
typedef Ind_dict Asm_locs;

typedef struct {
  Asm_heap       write; // Writable code heap.
  Asm_heap       exec;  // Executable code heap.
  Asm_fixup_list fixup; // Used as scratch in word compilation.
  Asm_got        got;   // Mapping of intrin/extern symbols to GOT entries.
  Asm_locs       locs;  // Used as scratch for "locals".
} Asm;

// All fields are offsets from `&Asm.write.page->instrs`.
typedef struct {
  Ind prologue; // Setup instructions; some may be skipped.
  Ind begin;    // Actual start; somewhere in `[prologue,inner]`.
  Ind inner;    // First useful instruction.
  Ind epilogue; // Cleanup instructions; may be `== .ret`.
  Ind ret;      // Always `ret`.
  Ind data;     // Local immediate values.
  Ind next;     // Start of next symbol's instructions.
} Sym_instrs;

/*
In instruction encoding, registers 0-31 are represented with corresponding 5-bit
unsigned integers. This applies to GPRs and vector/float registers. For example,
`x31` and `q31` are `0b11111`.
*/

static constexpr U8 ASM_PARAM_REG_LEN = 8;
static constexpr U8 ASM_REG_LEN       = 32;

static constexpr U8 ASM_PARAM_REG_0 = 0;
static constexpr U8 ASM_PARAM_REG_1 = 1;
static constexpr U8 ASM_PARAM_REG_2 = 2;
static constexpr U8 ASM_PARAM_REG_3 = 3;
static constexpr U8 ASM_PARAM_REG_4 = 4;
static constexpr U8 ASM_PARAM_REG_5 = 5;
static constexpr U8 ASM_PARAM_REG_6 = 6;
static constexpr U8 ASM_PARAM_REG_7 = 7;

static constexpr U8 ASM_SCRATCH_REG_8  = 8;
static constexpr U8 ASM_SCRATCH_REG_9  = 9;
static constexpr U8 ASM_SCRATCH_REG_10 = 10;
static constexpr U8 ASM_SCRATCH_REG_11 = 11;
static constexpr U8 ASM_SCRATCH_REG_12 = 12;
static constexpr U8 ASM_SCRATCH_REG_13 = 13;
static constexpr U8 ASM_SCRATCH_REG_14 = 14;
static constexpr U8 ASM_SCRATCH_REG_15 = 15;

// `x0` = "exception" register; hardcoded in a few places.
static constexpr U8 ASM_ERR_REG = ASM_PARAM_REG_0;

// Pointer to `Interp.ints.top`; hardcoded in a few places.
static constexpr U8 ASM_REG_INT_FLOOR = 26;

// Pointer to `Interp.ints.top`; hardcoded in a few places.
static constexpr U8 ASM_REG_INT_TOP = 27;

// Pointer to `Interp`; hardcoded in a few places.
static constexpr U8 ASM_REG_INTERP = 28;

static constexpr U8 ASM_REG_FP = 29;

// Updated by `bl`/`blr` and used by `ret`.
static constexpr U8 ASM_REG_LINK = 30;

// System SP / ZR.
static constexpr U8 ASM_REG_SP = 31;

// Magic numbers for `brk` instructions. Makes them more identifiable.
typedef enum : Instr {
  ASM_CODE_PROC_DELIM = 1,
  ASM_CODE_PROLOGUE   = 2,
  ASM_CODE_RET_EPI    = 3,
  ASM_CODE_RECUR      = 4,
  ASM_CODE_LOAD       = 5,
  ASM_CODE_TRY        = 6,
} Asm_code;
