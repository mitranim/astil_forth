/*
## Links

- Arm64 ISA in HTML format (say NO to horrible PDFs):
  - https://developer.arm.com/Architectures/A-Profile%20Architecture#Downloads
  - Look for `A64` in HTML format.
- Procedure call standard: https://github.com/ARM-software/abi-aa/blob/c51addc3dc03e73a016a1e4edf25440bcac76431/aapcs64/aapcs64.rst
- Condition codes' encoding: see documentation of `B.cond`.
- Condition codes' explanation: https://developer.arm.com/documentation/dui0801/l/Condition-Codes/Condition-code-suffixes-and-related-flags
  - Interchangeable: "CS" = "HS", "CC" = "LO".
- LLVM's Arm64 codegen is defined in `.td` files here: https://github.com/llvm/llvm-project/tree/main/llvm/lib/Target/AArch64
  - Base instructions are in `AArch64InstrFormats.td`.
- Apple ABI: https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms
- System V ABI: https://github.com/ARM-software/abi-aa/blob/c51addc3dc03e73a016a1e4edf25440bcac76431/sysvabi64/sysvabi64.rst

## Calling convention

GPR usage in the procedure call standard:

- r0-r7   = volatile (caller-saved), used for input and output parameters.
- r8      = volatile, used for indirect output parameters.
- r9-r15  = volatile, local scratch.
- r16-r17 = reserved for debuggers.
- r18     = reserved by the OS.
- r19-r28 = stable (callee-saved), used for long-lived local variables.
- r29     = address of current frame record (on the stack).
- r30     = address of calling code ("link register" or "LR").
- r31     = reserved: stack pointer ("SP") or zero register ("ZR").

We also dedicate a few registers to special roles, depending on the
calling convention: stack vs regs (or "callvention" if you will).
See `./c_arch_arm64.h` for definitions.

## Misc

Since we only assemble for the current system, we don't have to think about
little endian vs big endian. Arm64 instructions are simply `uint32`, and we
get the correct byte order for free. If we supported cross-compilation we'd
have to make endianness dynamically configurable. All major systems are LE,
so the bother is not worth it.

We could encode instructions by using struct bitfields with a dedicated struct
type for each instruction type. It works and makes the code much clearer, but
bitfield layout is implementation-defined and non-portable.
*/
#pragma once
#include "./c_arch_arm64.h"
#include "./c_comp.h"
#include "./c_sym.c"
#include "./lib/dict.c"
#include "./lib/fmt.h"
#include "./lib/jit.c"
#include "./lib/list.c"
#include "./lib/stack.c"

static bool comp_code_is_instr_ours(const Comp_code *code, const Instr *addr) {
  return is_list_elem(&code->code_exec, addr);
}

/*
Prog counter is the next instruction in `Comp_heap.exec`.
The executable heap is outdated while assembling,
so we have to adjust it against the writable heap.
*/
static Instr *comp_code_next_prog_counter(const Comp_code *code) {
  return list_spare_ptr(&code->code_exec, code->code_write.len);
}

static U8 *comp_code_next_data(const Comp_code *code) {
  return list_next_ptr(&code->data);
}

static Instr *comp_code_next_writable_instr(const Comp_code *code) {
  return list_next_ptr(&code->code_write);
}

static const Instr *comp_sym_exec_instr(const Comp *comp, const Sym *sym) {
  IF_DEBUG(aver(sym->type == SYM_NORM));
  return list_elem_ptr(&comp->code.code_exec, sym->norm.spans.prologue);
}

static Err comp_code_sync(Comp_code *code) {
  const auto valid_len = code->valid_instr_len;
  const auto write     = &code->code_write;
  const auto exec      = &code->code_exec;
  const auto exec_len  = exec->len;
  const auto diff      = valid_len - exec_len;

  if (diff <= 0) return nullptr;

  const auto beg = &exec->dat[exec_len];
  const auto end = beg + diff;
  const auto len = (U8 *)end - (U8 *)beg;

  // This alignment is only needed when `./lib/jit.c` uses `mprotect`.
  const auto page_beg = __builtin_align_down(beg, MEM_PAGE);
  const auto page_end = __builtin_align_up(end, MEM_PAGE);
  const auto page_len = (U8 *)page_end - (U8 *)page_beg;

  IF_DEBUG({
    aver(page_beg >= exec->dat);
    aver(page_end <= exec->dat + exec->cap);
    aver(page_len > 0);
    aver(page_beg <= beg);
    aver(end <= page_end);
  });

  try(jit_before_write(page_beg, (Uint)page_len));
  memcpy(beg, &write->dat[exec_len], (Uint)len);
  try(jit_after_write(page_beg, (Uint)page_len));

  exec->len = valid_len;
  return nullptr;
}

static bool comp_code_is_sym_ready(const Comp_code *code, const Sym *sym) {
  IF_DEBUG(aver(sym->type == SYM_NORM));
  return code->code_exec.len >= sym->norm.spans.ceil;
}

static Err err_sym_not_ready(const char *name) {
  return errf(
    "internal error: unable to call " FMT_QUOTED
    ": instructions are not synced up",
    name
  );
}

static Err comp_code_ensure_sym_ready(Comp_code *code, const Sym *sym) {
  if (comp_code_is_sym_ready(code, sym)) return nullptr;
  try(comp_code_sync(code));
  if (comp_code_is_sym_ready(code, sym)) return nullptr;
  return err_sym_not_ready(sym->name.buf);
}

static Err err_imm_range_signed(Sint imm, Uint wid, Sint min, Sint max) {
  return errf(
    "value " FMT_SINT " out of signed " FMT_UINT "-bit range [" FMT_SINT
    "," FMT_SINT "]",
    imm,
    wid,
    min,
    max
  );
}

static Err err_imm_range_unsigned(Uint imm, Uint wid, Uint max) {
  return errf(
    "value " FMT_UINT " out of unsigned " FMT_UINT "-bit range [0," FMT_UINT "]",
    imm,
    wid,
    max
  );
}

/*
Verifies if the given unsigned immediate fits into the given bit range.
If OK, `instr | imm` for the lowest bits is safe.

Assumptions: `width < 32`, `sizeof(Sint) >= 32/8`.
*/
static Err imm_unsigned(Uint src, U8 wid) {
  const Uint max = ((Uint)1 << wid) - 1;
  if (src > max) {
    return err_imm_range_unsigned(src, wid, max);
  }
  return nullptr;
}

/*
Returns an error if the given signed number doesn't fit into the bit width.
Otherwise zeroes all bits beyond the given bit width. The resulting pattern
is safe to `|` into the given N bits of an instruction, where N matches the
given width, shifted as necessary, without clobbering other bits, even when
the original number is negative.

For example, `(int32_t)-32` looks like this:

  11111111111111111111111111100000

Truncating to 6 bits produces unsigned 32, but when encoded into a position
where a signed `imm6` is expected, the CPU will interpret it as -32:

  00000000000000000000000000100000

Assumptions: `width < 32`, `sizeof(Sint) >= 32/8`.
*/
static Err imm_signed(Sint src, U8 wid, Instr *out) {
  // Comments show example bit patterns with `wid = 6` within `U8`.
  const Sint sig = (Sint)1 << (wid - 1); // 00100000 = 32
  const Sint max = sig - 1;              // 00011111 = 31
  const Sint min = -max - 1;             // 11100000 = -32

  if (src < min || src > max) {
    return err_imm_range_signed(src, wid, min, max);
  }

  *out = ((Instr)src & (Instr)max) | (src < 0 ? (Instr)sig : 0);
  return nullptr;
}

static Err asm_validate_reg(Sint reg) {
  if (reg >= 0 && reg < ARCH_REG_LEN) return nullptr;
  return errf("invalid register value " FMT_SINT, reg);
}

/*
We assume we're calling a C procedure, and we're doing that from C
which limits us to just 1 output, despite more supported by Arm64.
We could get around that by using an inline asm call and listing a
giant clobber list, but there's no point since `libc` doesn't have
any procedures with multiple outputs.

Note: this works identically for both of our callventions.
*/
static Err arch_call_extern(Sint_stack *stack, const Sym *sym) {
  aver(sym->type == SYM_EXTERN);

  const auto fun     = (Extern_fun *)sym->exter;
  const auto inp_len = sym->inp_len;
  const auto out_len = sym->out_len;

  aver(inp_len <= ARCH_INP_PARAM_REG_LEN);
  aver(out_len <= 1);

  Sint x0 = 0;
  Sint x1 = 0;
  Sint x2 = 0;
  Sint x3 = 0;
  Sint x4 = 0;
  Sint x5 = 0;
  Sint x6 = 0;
  Sint x7 = 0;
  if (inp_len > 7) try(int_stack_pop(stack, &x7));
  if (inp_len > 6) try(int_stack_pop(stack, &x6));
  if (inp_len > 5) try(int_stack_pop(stack, &x5));
  if (inp_len > 4) try(int_stack_pop(stack, &x4));
  if (inp_len > 3) try(int_stack_pop(stack, &x3));
  if (inp_len > 2) try(int_stack_pop(stack, &x2));
  if (inp_len > 1) try(int_stack_pop(stack, &x1));
  if (inp_len > 0) try(int_stack_pop(stack, &x0));

  const Sint out = fun(x0, x1, x2, x3, x4, x5, x6, x7);
  if (out_len) try(int_stack_push(stack, out));
  return nullptr;
}

static Instr *asm_append_instr(Comp *comp, Instr val) {
  return list_push(&comp->code.code_write, val);
}

/*
Base pattern used by:

  ldur
  stur
  ldr Xt, [Xn, <imm>]!
  ldr Xt, [Xn], <imm>
*/
static Instr ASM_BASE_LOAD_STORE = 0b11'111'0'00'00'0'000000000'00'00000'00000;

/*
Variants:

  opc 00 = str
  opc 01 = ldr

Pre/post depends on the sign:

  str val_reg, [addr_reg, -pre_mod]! // full descending
  ldr val_reg, [addr_reg], +post_mod // full descending

  str val_reg, [addr_reg], +post_mod // empty ascending
  ldr val_reg, [addr_reg, -pre_mod]! // empty ascending
*/
static void asm_append_load_store_pre_post(
  Comp *comp, bool is_load, U8 val_reg, U8 addr_reg, Sint mod
) {
  Instr order = mod < 0 ? 0b11 : 0b01;
  Instr mod_val;

  averr(imm_signed(mod, 9, &mod_val));
  averr(asm_validate_reg(val_reg));
  averr(asm_validate_reg(addr_reg));

  asm_append_instr(
    comp,
    (Instr)ASM_BASE_LOAD_STORE | ((Instr)is_load << 22) | (mod_val << 12) |
      (order << 10) | ((Instr)addr_reg << 5) | val_reg
  );
}

static void asm_append_store_pre_post(
  Comp *comp, U8 src_reg, U8 addr_reg, Sint mod_val
) {
  asm_append_load_store_pre_post(comp, false, src_reg, addr_reg, mod_val);
}

static void asm_append_load_pre_post(
  Comp *comp, U8 out_reg, U8 addr_reg, Sint mod_val
) {
  asm_append_load_store_pre_post(comp, true, out_reg, addr_reg, mod_val);
}

/*
Variants:

  opc 00 = str
  opc 01 = ldr

  str <val_reg>, [<addr_reg>, <off>]
  ldr <val_reg>, [<addr_reg>, <off>]

Scaled offset can only be unsigned.
*/
static Instr asm_instr_load_store_scaled_offset(
  bool is_load, U8 val_reg, U8 addr_reg, Uint off
) {
  aver(divisible_by(off, 8));
  off /= 8;

  averr(imm_unsigned(off, 12));
  averr(asm_validate_reg(val_reg));
  averr(asm_validate_reg(addr_reg));

  return (Instr)0b11'111'0'01'00'000000000000'00000'00000 |
    ((Instr)is_load << 22) | ((Instr)off << 10) | ((Instr)addr_reg << 5) |
    val_reg;
}

static Instr asm_instr_load_scaled_offset(U8 src_reg, U8 addr_reg, Uint off) {
  return asm_instr_load_store_scaled_offset(true, src_reg, addr_reg, off);
}

static Instr asm_instr_store_scaled_offset(U8 out_reg, U8 addr_reg, Uint off) {
  return asm_instr_load_store_scaled_offset(false, out_reg, addr_reg, off);
}

static void asm_append_load_scaled_offset(
  Comp *comp, U8 out_reg, U8 addr_reg, Uint off
) {
  asm_append_instr(comp, asm_instr_load_scaled_offset(out_reg, addr_reg, off));
}

static void asm_append_store_scaled_offset(
  Comp *comp, U8 src_reg, U8 addr_reg, Uint off
) {
  asm_append_instr(comp, asm_instr_store_scaled_offset(src_reg, addr_reg, off));
}

/*
Variants:

  opc 00 = stur
  opc 01 = ldur

  stur <val_reg>, [<addr_reg>, <off>]
  ldur <val_reg>, [<addr_reg>, <off>]
*/
static Instr asm_instr_load_store_unscaled_offset(
  bool is_load, U8 val_reg, U8 addr_reg, Sint off
) {
  Instr imm;
  averr(imm_signed(off, 9, &imm));
  averr(asm_validate_reg(val_reg));
  averr(asm_validate_reg(addr_reg));

  return (Instr)ASM_BASE_LOAD_STORE | ((Instr)is_load << 22) |
    ((Instr)imm << 12) | ((Instr)addr_reg << 5) | val_reg;
}

static Instr asm_instr_load_unscaled_offset(U8 src_reg, U8 addr_reg, Sint off) {
  return asm_instr_load_store_unscaled_offset(true, src_reg, addr_reg, off);
}

static Instr asm_instr_store_unscaled_offset(U8 out_reg, U8 addr_reg, Sint off) {
  return asm_instr_load_store_unscaled_offset(false, out_reg, addr_reg, off);
}

// ldur <val_reg>, [<addr_reg>, <off>]
static void asm_append_load_unscaled_offset(
  Comp *comp, U8 out_reg, U8 addr_reg, Sint off
) {
  asm_append_instr(comp, asm_instr_load_unscaled_offset(out_reg, addr_reg, off));
}

// stur <val_reg>, [<addr_reg>, <off>]
static void asm_append_store_unscaled_offset(
  Comp *comp, U8 src_reg, U8 addr_reg, Sint off
) {
  asm_append_instr(comp, asm_instr_store_unscaled_offset(src_reg, addr_reg, off));
}

static void asm_append_load_literal_offset(Comp *comp, U8 reg, Sint off) {
  averr(asm_validate_reg(reg));

  Instr imm;
  averr(imm_signed(off, 19, &imm));

  asm_append_instr(
    comp,
    // ldr <reg>, <off>
    (Instr)0b01'011'0'00'0000000000000000000'00000 | (imm << 5) | reg
  );
}

/*
Appends `adrp <reg>, <imm>` which calculates an imprecise address and stores it
in `reg`; the resulting address is aligned to 4096 bytes. Returns the remaining
distance towards the desired address, counted in bytes, which should be used by
a subsequent instruction such as `ldr` or `add`.

The offset is pseudo-PC-relative and implicitly multiplied by 4096.
The instruction implicitly treats the low 12 bits of the PC as zeros.
The immediate has a unique encoding: its lowest 2 bits are placed
separately in the high bits of the instruction in the opcode region.
*/
static U16 asm_append_adrp(Comp *comp, U8 reg, Uint addr) {
  constexpr Uint bits      = 12;
  constexpr Uint mask      = (1 << bits) - 1;
  const auto     prog      = (Uint)(comp_code_next_prog_counter(&comp->code));
  const auto     pc_page   = prog & ~mask;
  const auto     addr_page = addr & ~mask;
  const auto     page_diff = (addr_page >> bits) - (pc_page >> bits);
  const auto     pageoff   = addr - addr_page;

  aver(addr > prog);
  aver(addr_page > pc_page);
  aver(page_diff > 0);
  aver(addr >= addr_page);
  aver(pageoff < (1 << bits));

  // The immediate can be 21-bit signed, but ours is always positive.
  averr(imm_unsigned(page_diff, 20));

  const Instr high = (Instr)page_diff >> 2;
  const Instr low  = (Instr)page_diff & 0b11;

  asm_append_instr(
    comp,
    // adrp <reg>, <imm>
    (Instr)0b1'00'100'00'0000000000000000000'00000 | (low << 29) | (high << 5) |
      reg
  );
  return (U16)pageoff;
}

/*
In prologue / epilogue, we can often allocate stack space for the frame record
and locals in one `stp` / `ldp` instruction with a pre/post index, whose range
is limited to [-512,504]. The frame record takes 16. When exceeding this size,
we have to use another instruction.

SYNC[asm_locals_sp_off].
*/
static constexpr Ind ASM_LOCALS_MAX_PAIR_OFF = 488;

/*
Variants:

  opc 0 = stp
  opc 1 = ldp

  stp reg0, reg1, [addr_reg, -pre_off]!
  stp reg0, reg1, [addr_reg], +post_off

  ldp reg0, reg1, [addr_reg, -pre_off]!
  ldp reg0, reg1, [addr_reg], +post_off

Unlike in `str` and `ldr`, the offset in `stp` and `ldp`
is implicitly a multiple of the register width in bytes.
*/
static Instr asm_instr_load_store_pair_pre_post(
  bool is_load, U8 reg0, U8 reg1, U8 addr_reg, Sint mod
) {
  aver(mod >= -512 && mod <= 504); // See Arm64 docs.
  aver(divisible_by(mod, 8));
  mod /= 8;

  Instr order = mod < 0 ? 0b011 : 0b001;
  Instr mod_val;

  averr(imm_signed(mod, 7, &mod_val));
  averr(asm_validate_reg(reg0));
  averr(asm_validate_reg(reg1));
  averr(asm_validate_reg(addr_reg));

  return (Instr)0b10'101'0'001'0'0000000'00000'00000'00000 | (order << 23) |
    ((Instr)is_load << 22) | (mod_val << 15) | ((Instr)reg1 << 10) |
    ((Instr)addr_reg << 5) | reg0;
}

static Instr asm_instr_store_pair_pre_post(
  U8 reg0, U8 reg1, U8 addr_reg, Sint mod
) {
  return asm_instr_load_store_pair_pre_post(0b0, reg0, reg1, addr_reg, mod);
}

static Instr asm_instr_load_pair_pre_post(
  U8 reg0, U8 reg1, U8 addr_reg, Sint mod
) {
  return asm_instr_load_store_pair_pre_post(0b1, reg0, reg1, addr_reg, mod);
}

static void asm_append_store_pair_pre_post(
  Comp *comp, U8 reg0, U8 reg1, U8 addr_reg, Sint mod
) {
  asm_append_instr(
    comp, asm_instr_store_pair_pre_post(reg0, reg1, addr_reg, mod)
  );
}

static void asm_append_load_pair_pre_post(
  Comp *comp, U8 reg0, U8 reg1, U8 addr_reg, Sint mod
) {
  asm_append_instr(comp, asm_instr_load_pair_pre_post(reg0, reg1, addr_reg, mod));
}

// The offset is PC-relative and implicitly times 4.
static Instr asm_instr_branch_to_offset(Sint off) {
  Instr imm;
  averr(imm_signed(off, 26, &imm));
  return (Instr)0b0'00'101'00000000000000000000000000 | imm;
}

static void asm_append_branch_to_offset(Comp *comp, Sint off) {
  asm_append_instr(comp, asm_instr_branch_to_offset(off));
}

/*
The instruction will contain a PC-relative offset of the instruction it's
going to "call". "PC" stands for "program counter": memory address of the
instruction we're about to encode.

Different CPU architectures have different notions of PC.
On Arm64, it's the instruction currently executing.

The offset is implicitly times 4 at the CPU level.

`bl <imm>`
*/
static Instr asm_instr_branch_link_to_offset(Sint pc_off) {
  Instr imm26;
  averr(imm_signed(pc_off, 26, &imm26));
  return (Instr)0b1'00'101'00000000000000000000000000 | imm26;
}

static void asm_append_branch_link_to_offset(Comp *comp, Sint pc_off) {
  asm_append_instr(comp, asm_instr_branch_link_to_offset(pc_off));
}

static void asm_append_branch_link_to_reg(Comp *comp, U8 reg) {
  averr(asm_validate_reg(reg));
  const auto base = (Instr)0b110'101'1'0'0'01'11111'0000'0'0'00000'00000;
  asm_append_instr(comp, base | ((Instr)reg << 5));
}

static Instr asm_compare_branch(U8 reg, Sint off, bool non_zero) {
  averr(asm_validate_reg(reg));

  Instr imm;
  averr(imm_signed(off, 19, &imm));

  // 0 = zero
  // 1 = zero not
  const auto op = (Instr) !!non_zero << 24;

  // (cbz|cbnz) <reg>, <off*4>
  return (Instr)0b1'011010'0'0000000000000000000'00000 | op | (imm << 5) | reg;
}

static Instr asm_compare_branch_zero(U8 reg, Sint off) {
  return asm_compare_branch(reg, off, false);
}

static Instr asm_compare_branch_non_zero(U8 reg, Sint off) {
  return asm_compare_branch(reg, off, true);
}

static void asm_append_compare_branch(
  Comp *comp, U8 reg, Sint off, bool non_zero
) {
  asm_append_instr(comp, asm_compare_branch(reg, off, non_zero));
}

static void asm_append_compare_branch_zero(Comp *comp, U8 reg, Sint off) {
  asm_append_instr(comp, asm_compare_branch_zero(reg, off));
}

static void asm_append_compare_branch_non_zero(Comp *comp, U8 reg, Sint off) {
  asm_append_instr(comp, asm_compare_branch_non_zero(reg, off));
}

static const Instr *asm_sym_epilogue_writable(const Comp *comp, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  return list_elem_ptr(&comp->code.code_write, sym->norm.spans.epilogue);
}

static const Instr *asm_sym_epilogue_executable(const Comp *comp, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  return list_elem_ptr(&comp->code.code_exec, sym->norm.spans.epilogue);
}

static Instr *asm_sym_prologue_writable(const Comp *comp, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  return list_elem_ptr(&comp->code.code_write, sym->norm.spans.prologue);
}

static Instr *asm_sym_prologue_executable(const Comp *comp, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  const auto list = &comp->code.code_exec;
  aver(sym->norm.spans.prologue < list->cap);
  return &list->dat[sym->norm.spans.prologue];
}

/*
We use breakpoint instructions with various magic codes
when reserving space for later fixups.
*/
static Instr asm_instr_breakpoint(Instr imm) {
  averr(imm_unsigned(imm, 16));
  // brk <imm>
  return (Instr)0b110'101'00'001'0000000000000000'000'00 | (imm << 5);
}

static Instr *asm_append_breakpoint(Comp *comp, Instr imm) {
  return asm_append_instr(comp, asm_instr_breakpoint(imm));
}

static void asm_fixup_ret(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(aver(fix->type == ASM_FIX_RET));

  const auto epi = asm_sym_epilogue_writable(comp, sym);
  const auto tar = fix->ret;
  const auto off = epi - tar;

  IF_DEBUG(aver(*tar == asm_instr_breakpoint(ASM_CODE_RET)));
  *tar = asm_instr_branch_to_offset(off);
}

static void asm_fixup_try(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(aver(fix->type == ASM_FIX_TRY));

  const auto epi = asm_sym_epilogue_writable(comp, sym);
  const auto tar = fix->try;
  const auto off = epi - tar;

  IF_DEBUG(aver(*tar == asm_instr_breakpoint(ASM_CODE_TRY)));
  *tar = asm_compare_branch_non_zero(ARCH_REG_ERR, off);
}

static void asm_fixup_recur(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(aver(fix->type == ASM_FIX_RECUR));

  const auto tar = fix->recur;
  const auto off = asm_sym_prologue_writable(comp, sym) - tar;

  IF_DEBUG(aver(*tar == asm_instr_breakpoint(ASM_CODE_RECUR)));
  *tar = asm_instr_branch_link_to_offset(off);
}

static void asm_fixup_load(Comp *comp, const Asm_fixup *fix, Sym *sym) {
  IF_DEBUG(aver(fix->type == ASM_FIX_IMM));

  const auto write = &comp->code.code_write;
  const auto imm   = &fix->imm;
  const auto pci   = imm->instr;
  const auto num   = imm->num;
  const auto imm32 = (U32)num;
  const auto top   = list_next_ptr(write);

  IF_DEBUG(aver(*pci == asm_instr_breakpoint(ASM_CODE_IMM)));

  Instr opc;
  if ((Uint)imm32 == (Uint)num) {
    opc = 0b00; // ldr <Wt>, <off>
    asm_append_instr(comp, imm32);
  }
  else {
    opc = 0b01; // ldr <Xt>, <off>
    list_push_raw_val(write, num);
  }

  Instr imm19;
  averr(imm_signed((top - pci), 19, &imm19));

  *pci = (Instr)0b00'011'0'00'0000000000000000000'00000 | (opc << 30) |
    (imm19 << 5) | imm->reg;
  sym->norm.has_loads = true;
}

static Instr asm_pattern_arith_imm(U8 tar_reg, U8 src_reg, Uint imm12) {
  averr(imm_unsigned(imm12, 12));
  averr(asm_validate_reg(src_reg));
  averr(asm_validate_reg(tar_reg));
  return ((Instr)imm12 << 10) | ((Instr)src_reg << 5) | tar_reg;
}

static Instr asm_instr_add_imm(U8 tar_reg, U8 src_reg, Uint imm12) {
  return (Instr)0b1'0'0'100010'0'000000000000'00000'00000 |
    asm_pattern_arith_imm(tar_reg, src_reg, imm12);
}

static Instr asm_instr_sub_imm(U8 tar_reg, U8 src_reg, Uint imm12) {
  return (Instr)0b1'1'0'100010'0'000000000000'00000'00000 |
    asm_pattern_arith_imm(tar_reg, src_reg, imm12);
}

static void asm_append_add_imm(Comp *comp, U8 tar_reg, U8 src_reg, Uint imm12) {
  asm_append_instr(comp, asm_instr_add_imm(tar_reg, src_reg, imm12));
}

static void asm_append_sub_imm(Comp *comp, U8 tar_reg, U8 src_reg, Uint imm12) {
  asm_append_instr(comp, asm_instr_sub_imm(tar_reg, src_reg, imm12));
}

static void comp_local_alloc_mem(Comp *comp, Local *loc) {
  aver(!loc->mem);
  loc->mem = comp->ctx.mem_locs++;
}

// SYNC[local_fp_off].
static Ind local_fp_off(Local *loc) {
  return 16 + (loc->mem * (Ind)sizeof(Sint));
}

// TODO: detect when the offset exceeds what we can encode inline
// in a `ldp` / `stp`, and fall back on different encoding.
static Ind asm_locals_sp_off(Comp *comp) {
  const auto len = comp->ctx.mem_locs;

  // On Arm64, SP must be aligned to 16 bytes.
  return __builtin_align_up((len * (Ind)sizeof(Sint)), 16);
}

/*
In the prologue, we may need to create a frame record, and/or adjust the SP
to reserve space for locals. Since the locals aren't known upfront, and the
SP adjustment may or may not be needed, we fixup the prologue when appending
the epilogue, at which point everything is known.

SYNC[asm_prologue_epilogue].
*/
static void asm_reserve_sym_prologue(Comp *comp) {
  asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
  asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
  asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
}

// SYNC[asm_prologue_epilogue].
static void asm_fixup_sym_prologue(Comp *comp, Sym *sym, Ind *instr_floor) {
  const auto instrs = &comp->code.code_write;
  const auto sp_off = asm_locals_sp_off(comp);
  const auto leaf   = is_sym_leaf(sym) && !sp_off;
  const auto spans  = &sym->norm.spans;
  const auto inner  = &instrs->dat[spans->inner];
  auto       floor  = inner;

  IF_DEBUG({
    const auto brk = asm_instr_breakpoint(ASM_CODE_PROLOGUE);
    const auto pro = &instrs->dat[spans->prologue];

    aver((inner - pro) == 3);
    aver(pro[0] == brk);
    aver(pro[1] == brk);
    aver(pro[2] == brk);
  });

  if (!leaf) {
    *--floor = asm_instr_add_imm(ARCH_REG_FP, ARCH_REG_SP, 0);
    if (sp_off <= ASM_LOCALS_MAX_PAIR_OFF) {
      *--floor = asm_instr_store_pair_pre_post(
        ARCH_REG_FP, ARCH_REG_LINK, ARCH_REG_SP, (Sint)-16 - (Sint)sp_off
      );
    }
    else {
      *--floor = asm_instr_store_pair_pre_post(
        ARCH_REG_FP, ARCH_REG_LINK, ARCH_REG_SP, -16
      );
      *--floor = asm_instr_sub_imm(ARCH_REG_SP, ARCH_REG_SP, sp_off);
    }
  }

  *instr_floor = (Ind)(floor - instrs->dat);
}

/*
Sometimes we speculatively reserve instructions immediately following the
already-reserved prologue. When such instructions are deemed unnecessary,
we can skip them, advancing the prologue, to avoid having to insert nops.
Speculative unclobbering assignment of params or locals is one such case.

SYNC[asm_prologue].
*/
static bool asm_skipped_prologue_instr(Comp *comp, Sym *sym, Instr *instr) {
  const auto spans = &sym->norm.spans;
  const auto write = &comp->code.code_write;
  if (&write->dat[spans->inner] != instr) return false;

  IF_DEBUG(*instr = asm_instr_breakpoint(ASM_CODE_PROLOGUE));
  spans->prologue++;
  spans->inner++;
  return true;
}

// SYNC[asm_prologue_epilogue].
static void asm_append_sym_epilogue(Comp *comp, Sym *sym) {
  const auto sp_off = asm_locals_sp_off(comp);
  const auto leaf   = is_sym_leaf(sym) && !sp_off;

  if (leaf) return;

  // SYNC[local_fp_off].
  if (sp_off <= ASM_LOCALS_MAX_PAIR_OFF) {
    asm_append_load_pair_pre_post(
      comp, ARCH_REG_FP, ARCH_REG_LINK, ARCH_REG_SP, sp_off + 16
    );
  }
  else {
    asm_append_add_imm(comp, ARCH_REG_SP, ARCH_REG_SP, sp_off);
    asm_append_load_pair_pre_post(
      comp, ARCH_REG_FP, ARCH_REG_LINK, ARCH_REG_SP, 16
    );
  }
}

/*
TODO use or drop.

Should be invoked immediately after encoding branch-with-link instructions,
namely `bl` and `blr`. The address of the next instruction becomes the value
of the link register `x30` stored in the next frame record. We could store
metadata for every return address and create backtraces when unwinding.

  static void asm_register_call(Comp *comp, const Sym *caller) {
    map_set(&comp->cfi, comp_code_next_prog_counter(comp), caller);
  }
*/

static void asm_register_call(Comp *, const Sym *) {}

static Instr asm_instr_mov_reg(U8 tar_reg, U8 src_reg) {
  averr(asm_validate_reg(tar_reg));
  averr(asm_validate_reg(src_reg));

  return (Instr)0b1'01'01010'00'0'00000'000000'11111'00000 |
    ((Instr)src_reg << 16) | (Instr)tar_reg;
}

static void asm_append_mov_reg(Comp *comp, U8 tar_reg, U8 src_reg) {
  asm_append_instr(comp, asm_instr_mov_reg(tar_reg, src_reg));
}

/*
The instruction "mov wide immediate" supports an optional left shift
which sometimes allows to encode larger immediates inline.
*/
static Instr asm_maybe_mov_imm_to_reg(
  Instr base, Instr hw, Uint imm, Uint imm_max, U8 reg
) {
  aver(hw && hw <= 0b11);
  averr(imm_unsigned(reg, 5));

  const auto shift = (Uint)hw << 4;
  const auto low   = ((Uint)1 << shift) - 1;
  if (imm & low) return 0;

  const auto scaled = imm >> shift;
  if (scaled > imm_max) return 0;

  return base | (hw << 21) | (Instr)(scaled << 5) | reg;
}

static void asm_append_imm_to_reg(Comp *comp, U8 reg, Sint src, bool *has_load) {
  averr(imm_unsigned(reg, 5));
  if (has_load) *has_load = true;

  const auto imm = src < 0 ? ~(Uint)src : (Uint)src;

  // movn = 0b00
  // movz = 0b10
  const auto opc = (Instr)(src < 0 ? 0b00 : 0b10);

  // imm16 unsigned
  constexpr auto imm_max = ((Uint)1 << 16) - 1;

  const auto base = (Instr)0b1'00'100101'00'0000000000000000'00000 |
    (opc << 29) | reg;

  if (imm <= imm_max) {
    asm_append_instr(comp, base | (Instr)((imm) << 5));
    if (has_load) *has_load = false;
    return;
  }

  Instr out = 0;
  if ((out = asm_maybe_mov_imm_to_reg(base, 0b01, imm, imm_max, reg))) {
    asm_append_instr(comp, out);
    if (has_load) *has_load = false;
    return;
  }
  if ((out = asm_maybe_mov_imm_to_reg(base, 0b10, imm, imm_max, reg))) {
    asm_append_instr(comp, out);
    if (has_load) *has_load = false;
    return;
  }
  if ((out = asm_maybe_mov_imm_to_reg(base, 0b11, imm, imm_max, reg))) {
    asm_append_instr(comp, out);
    if (has_load) *has_load = false;
    return;
  }

  stack_push(
    &comp->ctx.asm_fix,
    (Asm_fixup){
      .type      = ASM_FIX_IMM,
      .imm.instr = asm_append_breakpoint(comp, ASM_CODE_IMM),
      .imm.num   = src,
      .imm.reg   = reg,
    }
  );
}

static void asm_append_zero_reg(Comp *comp, U8 reg) {
  averr(asm_validate_reg(reg));
  const auto ireg  = (Instr)reg;
  const auto base  = (Instr)0b1'10'01010'00'0'00000'000000'00000'00000;
  const auto instr = base | (ireg << 16) | (ireg << 5) | ireg;
  asm_append_instr(comp, instr);
}

static void asm_append_try(Comp *comp) {
  stack_push(
    &comp->ctx.asm_fix,
    (Asm_fixup){
      .type = ASM_FIX_TRY,
      .try  = asm_append_breakpoint(comp, ASM_CODE_TRY),
    }
  );
}

static Err asm_append_call_norm(Comp *comp, Sym *caller, const Sym *callee) {
  try(comp_code_ensure_sym_ready(&comp->code, callee));

  const auto code   = &comp->code;
  const auto fun    = comp_sym_exec_instr(comp, callee);
  const auto pc_off = fun - comp_code_next_prog_counter(code);

  aver(comp_code_is_instr_ours(code, fun));
  asm_append_branch_link_to_offset(comp, pc_off);
  asm_register_call(comp, caller);
  if (callee->throws) asm_append_try(comp);

  IF_DEBUG(eprintf(
    "[system] in " FMT_QUOTED ": appended call to " FMT_QUOTED
    " with executable address %p and PC offset " FMT_SINT
    " (in instruction count)\n",
    caller->name.buf,
    callee->name.buf,
    fun,
    pc_off
  ));
  return nullptr;
}

static void asm_append_page_addr(Comp *comp, U8 reg, Uint adr) {
  const auto pageoff = asm_append_adrp(comp, reg, adr);
  if (pageoff) asm_append_add_imm(comp, reg, reg, pageoff);
}

static void asm_append_page_load(Comp *comp, U8 reg, Uint adr) {
  const auto pageoff = asm_append_adrp(comp, reg, adr);
  asm_append_load_scaled_offset(comp, reg, reg, pageoff);
}

static void asm_append_dysym_load(Comp *comp, const char *name, U8 reg) {
  const auto code = &comp->code;
  const auto inds = &code->gots.inds;
  aver(dict_has(inds, name));

  const auto got_ind = dict_get(&code->gots.inds, name);
  aver(ind_valid(got_ind));

  const auto got_addr = comp->code.heap->got + got_ind;
  asm_append_page_load(comp, reg, (Uint)got_addr);
}

// Simple, naive inlining without support for relocation.
static Err asm_inline_sym(Comp *comp, const Sym *sym) {
  try(validate_sym_inlinable(sym));

  const auto spans  = &sym->norm.spans;
  const auto instrs = &comp->code.code_write;
  const auto floor  = spans->inner;
  const auto ceil   = spans->ret;

  for (Ind ind = floor; ind < ceil; ind++) {
    list_push(instrs, instrs->dat[ind]);
  }
  if (sym->throws) asm_append_try(comp);

  IF_DEBUG({
    if (floor == ceil) {
      eprintf(
        "[system] skipped inlining " FMT_QUOTED ": zero useful instructions\n",
        sym->name.buf
      );
    }
    else {
      eprintf(
        "[system] inlined word " FMT_QUOTED "; instructions (" FMT_IND "):\n",
        sym->name.buf,
        ceil - floor
      );
      eprint_byte_range_hex((U8 *)&instrs->dat[floor], (U8 *)&instrs->dat[ceil]);
      fputc('\n', stderr);
    }
  });
  return nullptr;
}

#ifdef NATIVE_CALL_CONV
#include "./c_arch_arm64_cc_reg.c" // IWYU pragma: export
#else
#include "./c_arch_arm64_cc_stack.c" // IWYU pragma: export
#endif

static void asm_fixup(Comp *comp, Sym *sym) {
  for (stack_range(auto, fix, &comp->ctx.asm_fix)) {
    switch (fix->type) {
      case ASM_FIX_RET: {
        asm_fixup_ret(comp, fix, sym);
        continue;
      }
      case ASM_FIX_TRY: {
        asm_fixup_try(comp, fix, sym);
        continue;
      }
      case ASM_FIX_RECUR: {
        asm_fixup_recur(comp, fix, sym);
        continue;
      }
      case ASM_FIX_IMM: {
        asm_fixup_load(comp, fix, sym);
        continue;
      }
      default: unreachable();
    }
  }
}

static void asm_sym_beg(Comp *comp, Sym *sym) {
  const auto instrs = &comp->code.code_write;
  const auto spans  = &sym->norm.spans;

  spans->floor    = instrs->len;
  spans->prologue = instrs->len;
  asm_reserve_sym_prologue(comp);
  spans->inner = instrs->len;
}

static void asm_sym_end(Comp *comp, Sym *sym) {
  const auto write = &comp->code.code_write;
  const auto spans = &sym->norm.spans;

#ifdef NATIVE_CALL_CONV
  asm_fixup_locals(comp, sym);
#endif

  asm_fixup_sym_prologue(comp, sym, &spans->prologue);
  spans->epilogue = write->len;
  asm_append_sym_epilogue(comp, sym);
  spans->ret = write->len;
  asm_append_instr(comp, ASM_INSTR_RET);
  spans->data = write->len;
  asm_fixup(comp, sym);
  spans->ceil = write->len;

  // Execution never reaches this. Makes it easier to tell words apart.
  if (DEBUG) asm_append_breakpoint(comp, ASM_CODE_PROC_DELIM);

  const auto code       = &comp->code;
  code->valid_instr_len = write->len;
}
