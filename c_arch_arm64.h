#pragma once
#include "./lib/list.h"
#include "./lib/num.h"
#include "./lib/stack.h"

typedef U32 Instr;

// The Forth program passes `Instr` to the compiler in the form of `Sint`.
// The assembler also uses `Uint` internally for some values.
static_assert(sizeof(Sint) >= sizeof(S32));
static_assert(sizeof(Uint) >= sizeof(U32));

// The Forth program casts pointers to and from `Sint`.
static_assert(sizeof(void *) == sizeof(Sint));
static_assert(sizeof(void *) == sizeof(Uint));

typedef list_of(Instr) Instr_list;
typedef span_of(Instr) Instr_span;

// All fields are offsets in the instruction heaps.
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
Maximum register-only input/output count supported by C under Arm64 ABI.
Using this on procedures with fewer actual inputs or outputs is harmless.
*/
typedef Sint(Extern_fun)(Sint, Sint, Sint, Sint, Sint, Sint, Sint, Sint);

/*
In instruction encoding, registers 0-31 are represented with corresponding
5-bit unsigned integers. This applies to GPRs and vector/float registers.
For example, `x31` and `q31` are `0b11111`.
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

// Frame pointer register; holds address of `{x29, x30}`.
static constexpr U8 ASM_REG_FP = 29;

// Link register: return address; updated by `bl`/`blr` and used by `ret`.
static constexpr U8 ASM_REG_LINK = 30;

// System stack pointer; also treated as a null register by many instructions.
static constexpr U8 ASM_REG_SP = 31;

#ifdef NATIVE_CALL_ABI

static constexpr U8 ASM_ERR_REG    = 28;
static constexpr U8 ASM_REG_INTERP = 27;

#else // NATIVE_CALL_ABI

/*
Using `x0` as the error register exactly matches how we return errors in C.
When calling procs which don't return errors, we nullify the register after
the call.
*/
static constexpr U8 ASM_ERR_REG       = ASM_PARAM_REG_0;
static constexpr U8 ASM_REG_INTERP    = 28; // Interp
static constexpr U8 ASM_REG_INT_TOP   = 27; // Interp.ints.top
static constexpr U8 ASM_REG_INT_FLOOR = 26; // Interp.ints.floor

#endif // NATIVE_CALL_ABI

// Magic numbers for `brk` instructions. Makes them more identifiable.
typedef enum : Instr {
  ASM_CODE_RET = 1,
  ASM_CODE_TRY,
  ASM_CODE_IMM,
  ASM_CODE_RECUR,
  ASM_CODE_PROLOGUE,
  ASM_CODE_LOC_READ,
  ASM_CODE_LOC_WRITE,
  ASM_CODE_PROC_DELIM,
} Asm_magic;
