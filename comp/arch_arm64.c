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
See `./arch_arm64.h` for definitions.

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
#include "./arch_arm64.h"
#include "../clib/bits.c"
#include "../clib/dict.c"
#include "../clib/fmt.h"
#include "../clib/jit.c"
#include "../clib/list.c"
#include "../clib/misc.h"
#include "../clib/stack.c"
#include "./comp.h"
#include "./sym.c"

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
  IF_DEBUG(assert_fatal(sym->type == SYM_NORM));
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
  assert_fatal(len > 0 && len < IND_MAX);

  // This alignment is only needed when `../clib/jit.c` uses `mprotect`.
  const auto page_beg = __builtin_align_down(beg, MEM_PAGE);
  const auto page_end = __builtin_align_up(end, MEM_PAGE);
  const auto page_len = (U8 *)page_end - (U8 *)page_beg;
  assert_fatal(page_len > 0 && page_len < IND_MAX);

  IF_DEBUG({
    assert_fatal(page_beg >= exec->dat);
    assert_fatal(page_end <= exec->dat + exec->cap);
    assert_fatal(page_len > 0);
    assert_fatal(page_beg <= beg);
    assert_fatal(end <= page_end);
  });

  try(jit_before_write(page_beg, (Ind)page_len));
  memcpy(beg, &write->dat[exec_len], (Ind)len);
  try(jit_after_write(page_beg, (Ind)page_len));

  exec->len = valid_len;
  return nullptr;
}

static bool comp_code_is_sym_ready(const Comp_code *code, const Sym *sym) {
  IF_DEBUG(assert_fatal(sym->type == SYM_NORM));
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

static Err asm_validate_reg(Sint reg) {
  if (reg >= 0 && reg < ASM_REG_LEN) return nullptr;
  return errf("invalid register value " FMT_SINT, reg);
}

static Err err_too_many_params(const char *kind, Sint cap, Sint req) {
  return errf(
    "too many %s parameters: " FMT_SINT " registers available, " FMT_SINT
    " parameters requested",
    kind,
    cap,
    req
  );
}

static Err asm_validate_input_param_reg(Sint reg) {
  if (reg >= 0 && reg < ASM_INP_PARAM_REG_LEN) return nullptr;
  return err_too_many_params("input", ASM_INP_PARAM_REG_LEN, reg + 1);
}

static Err asm_validate_input_param_count(Sint count) {
  if (count >= 0 && count <= ASM_INP_PARAM_REG_LEN) return nullptr;
  return err_too_many_params("input", ASM_INP_PARAM_REG_LEN, count);
}

static Err asm_validate_output_param_reg(Sint reg) {
  if (reg >= 0 && reg < ASM_OUT_PARAM_REG_LEN) return nullptr;
  return err_too_many_params("output", ASM_OUT_PARAM_REG_LEN, reg + 1);
}

static Err asm_validate_output_param_count(Sint count) {
  if (count >= 0 && count <= ASM_OUT_PARAM_REG_LEN) return nullptr;
  return err_too_many_params("output", ASM_OUT_PARAM_REG_LEN, count);
}

/*
We use all available volatile registers as our "comptime stack"
for arguments. Note that we still follow the standard calling
convention, which restricts function _parameters_ to fewer.

TODO: this shouldn't always say "provided".
See `comp_cc_reg.c` which uses this internally
all over the place for different purposes.
Sometimes it would be "requested" or something.
*/
static Err err_too_many_args(Sint req) {
  return errf(
    "too many arguments: %d registers available, " FMT_SINT
    " arguments provided",
    ASM_ARG_LEN_MAX,
    req
  );
}

static Err asm_validate_arg_reg(Sint reg) {
  if (reg >= 0 && reg < ASM_ARG_LEN_MAX) return nullptr;
  return err_too_many_args(reg + 1);
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
  IF_DEBUG(assert_fatal(wid));

  // Comments show example bit patterns with `wid = 6` within `U8`.
  // Some redundant casts were a concession to `clang-tidy` checks.
  const auto sig = (Sint)((Uint)1 << (U8)(wid - (U8)1)); // 00100000 = 32
  const Sint max = sig - 1;                              // 00011111 = 31
  const Sint min = -max - 1;                             // 11100000 = -32

  if (src < min || src > max) {
    return err_imm_range_signed(src, wid, min, max);
  }

  *out = ((Instr)src & (Instr)max) | (src < 0 ? (Instr)sig : 0);
  return nullptr;
}

/*
We assume we're calling a C function, and we're doing that from C
which limits us to just 1 output, despite more supported by Arm64.
We could get around that by using an inline asm call and listing a
giant clobber list, but there's no point since `libc` doesn't have
any functions with multiple outputs.

Note: this works identically for both of our callventions.
*/
static Err asm_call_extern(Sint_stack *stack, const Sym *sym) {
  assert_fatal(sym->type == SYM_EXTERN);

  const auto fun     = (Extern_fun *)sym->exter;
  const auto inp_len = sym->inp_len;
  const auto out_len = sym->out_len;

  assert_fatal(inp_len <= ASM_INP_PARAM_REG_LEN);
  assert_fatal(out_len <= 1);

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

  // Unused inputs are harmless.
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
static Instr asm_instr_load_store_pre_post(
  bool is_load, U8 val_reg, U8 addr_reg, Sint mod
) {
  Instr order = mod < 0 ? 0b11 : 0b01;
  Instr mod_val;

  try_fatal(imm_signed(mod, 9, &mod_val));
  try_fatal(asm_validate_reg(val_reg));
  try_fatal(asm_validate_reg(addr_reg));

  return ASM_BASE_LOAD_STORE | ((Instr)is_load << 22u) | (mod_val << 12u) |
    (order << 10u) | ((Instr)addr_reg << 5u) | val_reg;
}

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
  asm_append_instr(
    comp, asm_instr_load_store_pre_post(is_load, val_reg, addr_reg, mod)
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
  assert_fatal(divisible_by(off, 8));
  off /= 8;

  try_fatal(imm_unsigned(off, 12));
  try_fatal(asm_validate_reg(val_reg));
  try_fatal(asm_validate_reg(addr_reg));

  return (Instr)0b11'111'0'01'00'000000000000'00000'00000 |
    ((Instr)is_load << 22u) | ((Instr)off << 10u) | ((Instr)addr_reg << 5u) |
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
  try_fatal(imm_signed(off, 9, &imm));
  try_fatal(asm_validate_reg(val_reg));
  try_fatal(asm_validate_reg(addr_reg));

  return ASM_BASE_LOAD_STORE | ((Instr)is_load << 22u) | (imm << 12u) |
    ((Instr)addr_reg << 5u) | val_reg;
}

static Instr asm_instr_load_unscaled_offset(U8 src_reg, U8 addr_reg, Sint off) {
  return asm_instr_load_store_unscaled_offset(true, src_reg, addr_reg, off);
}

// ldur <val_reg>, [<addr_reg>, <off>]
static void asm_append_load_unscaled_offset(
  Comp *comp, U8 out_reg, U8 addr_reg, Sint off
) {
  asm_append_instr(comp, asm_instr_load_unscaled_offset(out_reg, addr_reg, off));
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
  constexpr Uint mask      = (1u << bits) - 1u; // 0b111111111111 = 4095
  const auto     prog      = (Uint)(comp_code_next_prog_counter(&comp->code));
  const auto     pc_page   = prog & ~mask;
  const auto     addr_page = addr & ~mask;
  const auto     page_diff = (addr_page >> bits) - (pc_page >> bits);
  const auto     pageoff   = addr - addr_page;

  assert_fatal(addr > prog);
  assert_fatal(addr_page > pc_page);
  assert_fatal(page_diff > 0);
  assert_fatal(addr >= addr_page);
  assert_fatal(pageoff < (1u << bits));

  // The immediate can be 21-bit signed, but ours is always positive.
  try_fatal(imm_unsigned(page_diff, 20));

  const Instr high = (Instr)page_diff >> 2u;
  const Instr low  = (Instr)page_diff & 0b11u;

  asm_append_instr(
    comp,
    // adrp <reg>, <imm>
    (Instr)0b1'00'100'00'0000000000000000000'00000 | (low << 29u) |
      (high << 5u) | reg
  );
  return (U16)pageoff;
}

/*
In prologue / epilogue, we can often allocate stack space for the frame record
and locals in one `stp` / `ldp` instruction with a pre/post index, whose range
is limited to [-512,504]. The frame record takes 16. When exceeding this size,
we have to use another instruction.
*/
static constexpr USED Ind ASM_MAX_LOAD_STORE_PAIR_OFF = 504;

// Opcode at bit 22 for the `ldp` / `stp` instructions.
static constexpr USED Instr ASM_LOAD_PAIR_POST  = 0b001'1;
static constexpr USED Instr ASM_LOAD_PAIR_PRE   = 0b011'1;
static constexpr USED Instr ASM_LOAD_PAIR_OFF   = 0b010'1;
static constexpr USED Instr ASM_STORE_PAIR_POST = 0b001'0;
static constexpr USED Instr ASM_STORE_PAIR_PRE  = 0b011'0;
static constexpr USED Instr ASM_STORE_PAIR_OFF  = 0b010'0;

/*
Several variants of the `ldp` / `stp` instructions (with scaled offsets).
Variant is determined by the `opc` arg. See the `ASM_*_PAIR_*` constants.

  ldp reg0, reg1, [addr_reg], off
  ldp reg0, reg1, [addr_reg, off]!
  ldp reg0, reg1, [addr_reg, off]

  stp reg0, reg1, [addr_reg], off
  stp reg0, reg1, [addr_reg, off]!
  stp reg0, reg1, [addr_reg, off]

Unlike in `str` and `ldr`, the offset in `stp` and `ldp`
is implicitly a multiple of the register width in bytes.
*/
static Instr asm_instr_load_store_pair(
  Instr opc, U8 reg0, U8 reg1, U8 addr_reg, Sint off
) {
  assert_fatal(off >= -512 && off <= 504); // See Arm64 docs.
  assert_fatal(divisible_by(off, 8));
  off /= 8;

  Instr off_val;
  try_fatal(imm_signed(off, 7, &off_val));
  try_fatal(asm_validate_reg(reg0));
  try_fatal(asm_validate_reg(reg1));
  try_fatal(asm_validate_reg(addr_reg));

  return (Instr)0b10'101'0'001'0'0000000'00000'00000'00000 | (opc << 22u) |
    (off_val << 15u) | ((Instr)reg1 << 10u) | ((Instr)addr_reg << 5u) | reg0;
}

static void asm_append_load_store_pair(
  Comp *comp, Instr opc, U8 reg0, U8 reg1, U8 addr_reg, Sint off
) {
  asm_append_instr(
    comp, asm_instr_load_store_pair(opc, reg0, reg1, addr_reg, off)
  );
}

// The offset is PC-relative and implicitly times 4.
static Instr asm_instr_branch_to_offset(Sint off) {
  Instr imm;
  try_fatal(imm_signed(off, 26, &imm));
  return (Instr)0b0'00'101'00000000000000000000000000 | imm;
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
  try_fatal(imm_signed(pc_off, 26, &imm26));
  return (Instr)0b1'00'101'00000000000000000000000000 | imm26;
}

static void asm_append_branch_link_to_offset(Comp *comp, Sint pc_off) {
  asm_append_instr(comp, asm_instr_branch_link_to_offset(pc_off));
}

static void asm_append_branch_link_to_reg(Comp *comp, U8 reg) {
  try_fatal(asm_validate_reg(reg));
  const auto base = (Instr)0b110'101'1'0'0'01'11111'0000'0'0'00000'00000;
  asm_append_instr(comp, base | ((Instr)reg << 5u));
}

static Instr asm_instr_compare_branch(U8 reg, Sint off, bool non_zero) {
  try_fatal(asm_validate_reg(reg));

  Instr imm;
  try_fatal(imm_signed(off, 19, &imm));

  // 0 = zero
  // 1 = zero not
  const auto op = (Instr) !!non_zero << 24u;

  // (cbz|cbnz) <reg>, <off*4>
  return (Instr)0b1'011010'0'0000000000000000000'00000 | op | (imm << 5u) | reg;
}

static Instr asm_instr_compare_branch_zero(U8 reg, Sint off) {
  return asm_instr_compare_branch(reg, off, false);
}

static Instr asm_instr_compare_branch_non_zero(U8 reg, Sint off) {
  return asm_instr_compare_branch(reg, off, true);
}

static void asm_append_compare_branch_zero(Comp *comp, U8 reg, Sint off) {
  asm_append_instr(comp, asm_instr_compare_branch_zero(reg, off));
}

static Instr *asm_sym_prologue_writable(const Comp *comp, const Sym *sym) {
  assert_fatal(sym->type == SYM_NORM);
  return list_elem_ptr(&comp->code.code_write, sym->norm.spans.prologue);
}

static Instr *asm_sym_prologue_executable(const Comp *comp, const Sym *sym) {
  assert_fatal(sym->type == SYM_NORM);
  const auto list = &comp->code.code_exec;
  assert_fatal(sym->norm.spans.prologue < list->cap);
  return &list->dat[sym->norm.spans.prologue];
}

/*
We use breakpoint instructions with various magic codes
when reserving space for later fixups.
*/
static Instr asm_instr_breakpoint(Instr imm) {
  try_fatal(imm_unsigned(imm, 16));
  // brk <imm>
  return (Instr)0b110'101'00'001'0000000000000000'000'00 | (imm << 5u);
}

static Instr *asm_append_breakpoint(Comp *comp, Instr imm) {
  return asm_append_instr(comp, asm_instr_breakpoint(imm));
}

static void asm_fixup_ret(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(assert_fatal(fix->type == ASM_FIX_RET));
  IF_DEBUG(assert_fatal(sym->type == SYM_NORM));

  const auto code = &comp->code.code_write;
  const auto epi  = list_elem_ptr(code, sym->norm.spans.epi_ok);
  const auto tar  = fix->ret;
  const auto off  = epi - tar;

  IF_DEBUG(assert_fatal(*tar == asm_instr_breakpoint(ASM_CODE_RET)));
  *tar = asm_instr_branch_to_offset(off);
}

static void asm_fixup_recur(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(assert_fatal(fix->type == ASM_FIX_RECUR));

  const auto tar = fix->recur;
  const auto off = asm_sym_prologue_writable(comp, sym) - tar;

  IF_DEBUG(assert_fatal(*tar == asm_instr_breakpoint(ASM_CODE_RECUR)));
  *tar = asm_instr_branch_link_to_offset(off);
}

static void asm_fixup_try(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(assert_fatal(fix->type == ASM_FIX_TRY));
  IF_DEBUG(assert_fatal(sym->type == SYM_NORM));

  const auto code  = &comp->code.code_write;
  const auto epi   = list_elem_ptr(code, sym->norm.spans.epi_err);
  const auto instr = fix->try.instr;
  const auto off   = epi - instr;

  IF_DEBUG(assert_fatal(*instr == asm_instr_breakpoint(ASM_CODE_TRY)));
  *instr = asm_instr_compare_branch_non_zero(fix->try.err_reg, off);
}

static Instr asm_pattern_arith_imm(U8 tar_reg, U8 src_reg, Uint imm12) {
  try_fatal(imm_unsigned(imm12, 12));
  try_fatal(asm_validate_reg(src_reg));
  try_fatal(asm_validate_reg(tar_reg));
  return (Instr)tar_reg | ((Instr)src_reg << 5u) | ((Instr)imm12 << 10u);
}

static constexpr Uint ASM_ADD_SUB_SCALE = 12;

static Instr asm_instr_add_imm(U8 tar_reg, U8 src_reg, Uint imm) {
  Instr shift = 0;

  if (imm && low_bits_zero(imm, ASM_ADD_SUB_SCALE)) {
    shift = 1;
    imm >>= ASM_ADD_SUB_SCALE;
  }

  return (Instr)0b1'0'0'100010'0'000000000000'00000'00000 | (shift << 22u) |
    asm_pattern_arith_imm(tar_reg, src_reg, imm);
}

static Instr asm_instr_sub_imm(U8 tar_reg, U8 src_reg, Uint imm) {
  Instr shift = 0;

  if (imm && low_bits_zero(imm, ASM_ADD_SUB_SCALE)) {
    shift = 1;
    imm >>= ASM_ADD_SUB_SCALE;
  }

  return (Instr)0b1'1'0'100010'0'000000000000'00000'00000 | (shift << 22u) |
    asm_pattern_arith_imm(tar_reg, src_reg, imm);
}

// Also see `asm_append_add` which is smarter.
static void asm_append_add_imm(Comp *comp, U8 tar_reg, U8 src_reg, Uint imm) {
  asm_append_instr(comp, asm_instr_add_imm(tar_reg, src_reg, imm));
}

static void asm_append_sub_imm(Comp *comp, U8 tar_reg, U8 src_reg, Uint imm) {
  asm_append_instr(comp, asm_instr_sub_imm(tar_reg, src_reg, imm));
}

// Shared by some integer arithmetic instructions.
static Instr asm_pattern_arith_reg(U8 tar_reg, U8 src_reg, U8 mod_reg) {
  try_fatal(asm_validate_reg(src_reg));
  try_fatal(asm_validate_reg(tar_reg));
  try_fatal(asm_validate_reg(mod_reg));
  return (Instr)tar_reg | ((Instr)src_reg << 5u) | ((Instr)mod_reg << 16u);
}

// add Xd, Xt|sp, Xm
static Instr asm_instr_add_reg(U8 tar_reg, U8 src_reg, U8 mod_reg) {
  return (Instr)0b1'0'0'01011'00'0'00000'000000'00000'00000 |
    asm_pattern_arith_reg(tar_reg, src_reg, mod_reg);
}

// sub Xd, Xt|sp, Xm
static Instr asm_instr_sub_reg_ext(U8 tar_reg, U8 src_reg, U8 mod_reg) {
  return (Instr)0b1'1'0'01011'00'1'00000'011'000'00000'00000 |
    asm_pattern_arith_reg(tar_reg, src_reg, mod_reg);
}

static void asm_append_add_reg(Comp *comp, U8 tar_reg, U8 src_reg, U8 mod_reg) {
  asm_append_instr(comp, asm_instr_add_reg(tar_reg, src_reg, mod_reg));
}

static void asm_append_sub_reg_ext(
  Comp *comp, U8 tar_reg, U8 src_reg, U8 mod_reg
) {
  asm_append_instr(comp, asm_instr_sub_reg_ext(tar_reg, src_reg, mod_reg));
}

// Arm64 requires the SP to be aligned to 16 when loading or storing.
static Ind asm_align_sp_off(Ind off) { return __builtin_align_up(off, 16); }

// and <reg>, <reg>, 0xfffffffffffffff0
static void asm_append_sp_align(Comp *comp, U8 reg) {
  try_fatal(asm_validate_reg(reg));
  asm_append_instr(
    comp,
    (Instr)0b1'00'100100'1'111100'111011'00000'00000 | ((Instr)reg << 5u) | reg
  );
}

/*
Scaling by 4096 allows to encode the offset inline in `add` and `sub`
up to 16-ish megabytes, which should be enough for real-world stacks.
Clang does similar stuff.

SYNC[asm_sp_off].
*/
static Ind asm_sp_off(Ind off) {
  off = asm_align_sp_off(off);
  if (high_bits_zero(off, ASM_ADD_SUB_SCALE)) return off;
  return __builtin_align_up(off, ((U8)1 << ASM_ADD_SUB_SCALE));
}

// SYNC[asm_sp_off].
static void comp_local_alloc_mem(Comp *comp, Local *loc) {
  assert_fatal(!loc->fp_off);

  const auto ctx = &comp->ctx;
  auto       off = ctx->fp_off;

  // Since locals use FP-relative addressing, they also require
  // a frame record. When the total FP offset is small enough,
  // we adjust the SP in one `ldr` / `str` instruction, which
  // results in the FP being at the bottom of the frame, rather
  // than at the top. So the frame is included in the offsets.
  if (!off) off += ASM_FRAME_RECORD_SIZE;

  loc->fp_off = off;
  ctx->fp_off = off + sizeof(Sint);
}

/*
In the prologue, we may need to create a frame record, and/or adjust the SP
to reserve space for locals. Since the locals aren't known upfront, and the
SP adjustment may or may not be needed, we fixup the prologue when appending
the epilogue, at which point everything is known.

SYNC[asm_prologue_epilogue].
*/
static void asm_reserve_sym_prologue(Comp *comp) {
#ifndef CALL_CONV_STACK

  // Reserve space for `stp` of callee-saved registers.
  auto len = __builtin_align_up((S8)ASM_STABLE_REG_LEN, 2);
  while ((len -= 2) >= 0) {
    asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
  }

#endif // CALL_CONV_STACK

  asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
  asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
  asm_append_breakpoint(comp, ASM_CODE_PROLOGUE);
}

// SYNC[asm_prologue_epilogue].
static void asm_fixup_sym_prologue(Comp *comp, Sym *sym, Ind *instr_floor) {
  const auto code   = &comp->code;
  const auto ctx    = &comp->ctx;
  const auto instrs = &code->code_write;
  const auto sp_off = asm_sp_off(ctx->fp_off);
  const auto spans  = &sym->norm.spans;
  const auto inner  = &instrs->dat[spans->inner];
  const bool frame  = !is_sym_leaf(sym) || sp_off || sym->norm.has_alloca;
  auto       floor  = inner;

#ifdef CALL_CONV_STACK
  static constexpr U8 len = 3;
#else
  static constexpr U8 len = 3 + (__builtin_align_up(ASM_STABLE_REG_LEN, 2) / 2);
#endif

  IF_DEBUG({
    const auto brk = asm_instr_breakpoint(ASM_CODE_PROLOGUE);
    const auto pro = &instrs->dat[spans->prologue];

    assert_fatal((inner - pro) == len);
    for (U8 ind = 0; ind < len; ind++) assert_fatal(pro[ind] == brk);
  });

  /*
  We generally prefer to skip frame records in leaf functions,
  but if any locals are used, we have to create one, because we
  address them relatively to the FP. A smarter compiler would
  switch to SP-relative addressing, but that would require more
  book-keeping and probably introduce new bugs. Maybe later.

  Note that the instructions are added in reverse order.

  SYNC[asm_sp_off].
  */
  if (frame) {
    const auto off = sp_off ? sp_off : ASM_FRAME_RECORD_SIZE;

    // mov x29, sp
    *--floor = asm_instr_add_imm(ASM_REG_FP, ASM_REG_SP, 0);

    if (off <= ASM_MAX_LOAD_STORE_PAIR_OFF) {
      // stp x29, x30, [sp, -<off>]!
      *--floor = asm_instr_load_store_pair(
        ASM_STORE_PAIR_PRE, ASM_REG_FP, ASM_REG_LINK, ASM_REG_SP, -(Sint)off
      );
    }
    else {
      // stp x29, x30, [sp]
      *--floor = asm_instr_load_store_pair(
        ASM_STORE_PAIR_OFF, ASM_REG_FP, ASM_REG_LINK, ASM_REG_SP, 0
      );
      // sub sp, sp, <off>
      *--floor = asm_instr_sub_imm(ASM_REG_SP, ASM_REG_SP, off);
    }
  }

#ifndef CALL_CONV_STACK

  // Stash callee-saved registers to memory.
  auto saved = ctx->saved_reg;

  while (saved > ASM_STABLE_REG_FIRST) {
    // stp <reg0>, <reg1>, [sp, -16]!
    *--floor = asm_instr_load_store_pair(
      ASM_STORE_PAIR_PRE, saved - 1, saved, ASM_REG_SP, -16
    );
    saved -= 2;
  }

  if (saved == ASM_STABLE_REG_FIRST) {
    // str <reg>, [sp, -16]!
    *--floor = asm_instr_load_store_pre_post(false, saved, ASM_REG_SP, -16);
  }

#endif // CALL_CONV_STACK

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

  *instr = asm_instr_breakpoint(ASM_CODE_PROLOGUE);

  spans->prologue++;
  spans->inner++;
  return true;
}

// TODO restructure to avoid the need for forward declarations.
static void asm_append_sym_epilogue_ok(Comp *, Sym *);

// SYNC[asm_prologue_epilogue].
static void asm_append_sym_epilogue(Comp *comp, Sym *sym) {
  const auto sp_off = asm_sp_off(comp->ctx.fp_off);
  const bool frame  = !is_sym_leaf(sym) || sp_off || sym->norm.has_alloca;
  const auto spans  = &sym->norm.spans;
  const auto write  = &comp->code.code_write;

  spans->epi_ok = write->len;
  asm_append_sym_epilogue_ok(comp, sym);
  spans->epi_err = write->len;

  // SYNC[asm_sp_off].
  if (frame) {
    if (sym->norm.has_alloca) {
      // mov sp, x29
      asm_append_add_imm(comp, ASM_REG_SP, ASM_REG_FP, 0);
    }

    const auto off = sp_off ? sp_off : ASM_FRAME_RECORD_SIZE;

    if (off <= ASM_MAX_LOAD_STORE_PAIR_OFF) {
      // ldp x29, x30, [sp], <off>
      asm_append_load_store_pair(
        comp, ASM_LOAD_PAIR_POST, ASM_REG_FP, ASM_REG_LINK, ASM_REG_SP, off
      );
    }
    else {
      // add sp, sp, <off>
      asm_append_add_imm(comp, ASM_REG_SP, ASM_REG_SP, off);

      // ldp x29, x30, [sp], <off>
      asm_append_load_store_pair(
        comp, ASM_LOAD_PAIR_OFF, ASM_REG_FP, ASM_REG_LINK, ASM_REG_SP, 0
      );
    }
  }

#ifndef CALL_CONV_STACK

  const auto ctx = &comp->ctx;

  // Restore callee-saved registers from memory.
  auto saved = ctx->saved_reg;

  while (saved > ASM_STABLE_REG_FIRST) {
    // ldp <reg0>, <reg1>, [sp], 16
    asm_append_load_store_pair(
      comp, ASM_LOAD_PAIR_POST, saved - 1, saved, ASM_REG_SP, 16
    );
    saved -= 2;
  }

  if (saved == ASM_STABLE_REG_FIRST) {
    // ldr <reg>, [sp], 16
    asm_append_load_store_pre_post(comp, true, saved, ASM_REG_SP, 16);
  }

#endif // CALL_CONV_STACK
}

static Instr asm_instr_mov_reg(U8 tar_reg, U8 src_reg) {
  try_fatal(asm_validate_reg(tar_reg));
  try_fatal(asm_validate_reg(src_reg));

  return (Instr)0b1'01'01010'00'0'00000'000000'11111'00000 |
    ((Instr)src_reg << 16u) | (Instr)tar_reg;
}

// mov Xd, Xt
static void asm_append_mov_reg(Comp *comp, U8 tar_reg, U8 src_reg) {
  asm_append_instr(comp, asm_instr_mov_reg(tar_reg, src_reg));
}

/*
ARM64 move-wide instructions encode one 16-bit lane. `movz` and `movn`
initialize all lanes to zero or ones; `movk` replaces one lane. This emits
any 64-bit immediate inline in at most four instructions.

SYNC[asm_imm_to_reg].
*/
static Instr asm_instr_mov_wide(U8 reg, Instr opc, Uint hw, Uint imm16) {
  try_fatal(imm_unsigned(reg, 5));
  try_fatal(imm_unsigned(opc, 2));
  assert_fatal(hw <= 0b11);
  try_fatal(imm_unsigned(imm16, 16));

  return (Instr)0b1'00'100101'00'0000000000000000'00000 | (opc << 29u) |
    ((Instr)hw << 21u) | ((Instr)imm16 << 5u) | reg;
}

static void asm_append_mov_wide(
  Comp *comp, U8 reg, Instr opc, Uint hw, Uint imm16
) {
  asm_append_instr(comp, asm_instr_mov_wide(reg, opc, hw, imm16));
}

static Uint asm_imm16_lane(Uint val, Uint hw) {
  assert_fatal(hw <= 0b11);
  return (val >> (hw << 4u)) & 0xFFFFu;
}

static bool asm_mov_wide_lane_is_default(bool neg, Uint lane) {
  return lane == (neg ? 0xFFFFu : 0);
}

static Uint asm_mov_wide_base_imm16(bool neg, Uint lane) {
  return neg ? (~lane & 0xFFFFu) : lane;
}

static void asm_append_imm_to_reg(Comp *comp, U8 reg, Sint src) {
  const bool neg = src < 0;
  const auto val = (Uint)src;
  const auto opc = (Instr)(neg ? 0b00 : 0b10); // movn / movz

  Uint base_hw = 0;
  while (
    base_hw < 4 &&
    asm_mov_wide_lane_is_default(neg, asm_imm16_lane(val, base_hw))
  ) {
    base_hw++;
  }
  if (base_hw >= 4) base_hw = 0;

  asm_append_mov_wide(
    comp,
    reg,
    opc,
    base_hw,
    asm_mov_wide_base_imm16(neg, asm_imm16_lane(val, base_hw))
  );

  for (Uint hw = 0; hw < 4; hw++) {
    const auto lane = asm_imm16_lane(val, hw);
    if (hw == base_hw || asm_mov_wide_lane_is_default(neg, lane)) continue;

    asm_append_mov_wide(comp, reg, 0b11, hw, lane); // movk
  }
}

static void asm_append_add(Comp *comp, U8 tar_reg, U8 src_reg, Sint imm) {
  if (imm >= 0 && imm < 4096) {
    asm_append_add_imm(comp, tar_reg, src_reg, (Uint)imm);
  }
  else {
    asm_append_imm_to_reg(comp, tar_reg, imm);
    asm_append_add_reg(comp, tar_reg, src_reg, tar_reg);
  }
}

static void asm_append_zero_reg(Comp *comp, U8 reg) {
  try_fatal(asm_validate_reg(reg));

  // eor <reg>, <reg>, <reg>
  const auto ireg  = (Instr)reg;
  const auto base  = (Instr)0b1'10'01010'00'0'00000'000000'00000'00000;
  const auto instr = base | (ireg << 16u) | (ireg << 5u) | ireg;
  asm_append_instr(comp, instr);
}

static void asm_append_fixup_try(Comp *comp, U8 reg) {
  stack_push(
    &comp->ctx.asm_fix,
    (Asm_fixup){
      .type        = ASM_FIX_TRY,
      .try.err_reg = reg,
      // cbnz <err>, <epi_err>
      .try.instr = asm_append_breakpoint(comp, ASM_CODE_TRY),
    }
  );
}

static void asm_append_fixup_throw(Comp *comp) {
  stack_push(
    &comp->ctx.asm_fix,
    (Asm_fixup){
      .type  = ASM_FIX_THROW,
      .throw = asm_append_breakpoint(comp, ASM_CODE_THROW), // b <err_epi>
    }
  );
}

static void asm_append_page_addr(Comp *comp, U8 reg, Uint adr) {
  const auto pageoff = asm_append_adrp(comp, reg, adr);
  if (pageoff) asm_append_add_imm(comp, reg, reg, pageoff);
}

static void asm_append_page_load(Comp *comp, U8 reg, Uint adr) {
  const auto pageoff = asm_append_adrp(comp, reg, adr);
  asm_append_load_scaled_offset(comp, reg, reg, pageoff);
}

static void asm_append_dysym_load(
  Comp *comp, const char *name, U8 reg, Comp_syms *syms
) {
  const auto inds    = &syms->inds;
  const auto got_ind = dict_get_or(inds, name, INVALID_IND);
  assert_fatal(got_ind != INVALID_IND);

  const auto got_addr = syms->addrs.dat + got_ind;
  asm_append_page_load(comp, reg, (Uint)got_addr);
}

static Err err_catch_no_throw(const char *callee) {
  return errf(
    "useless attempt to catch an error when calling word " FMT_QUOTED
    " which does not throw",
    callee
  );
}

#ifndef CALL_CONV_STACK
#include "./arch_arm64_cc_reg.c" // IWYU pragma: export
#else
#include "./arch_arm64_cc_stack.c" // IWYU pragma: export
#endif

static Err asm_append_call_norm(
  Comp *comp, Sym *caller, const Sym *callee, bool err_mode
) {
  const auto code = &comp->code;
  try(comp_code_ensure_sym_ready(code, callee));

  const auto fun    = comp_sym_exec_instr(comp, callee);
  const auto pc_off = fun - comp_code_next_prog_counter(code);

  assert_fatal(comp_code_is_instr_ours(code, fun));
  asm_append_branch_link_to_offset(comp, pc_off);
#ifdef CALL_CONV_STACK
  try(asm_append_try_catch(comp, caller, callee, err_mode));
#else
  (void)err_mode;
#endif

  IF_DEBUG(eprintf(
    "[system] in " FMT_QUOTED ": appended call to " FMT_QUOTED
    " with executable address %p and PC offset " FMT_SINT "\n",
    caller->name.buf,
    callee->name.buf,
    fun,
    (Sint)MUL(pc_off, sizeof(Instr))
  ));
  return nullptr;
}

// Simple, naive inlining without support for relocation.
static Err asm_inline_sym(
  Comp *comp, Sym *caller, const Sym *callee, bool err_mode
) {
  try(validate_sym_inlinable(callee));

  const auto spans  = &callee->norm.spans;
  const auto instrs = &comp->code.code_write;
  const auto floor  = spans->prologue;
  const auto ceil   = spans->ret;

  for (Ind ind = floor; ind < ceil; ind++) {
    list_push(instrs, instrs->dat[ind]);
  }

#ifdef CALL_CONV_STACK
  try(asm_append_try_catch(comp, caller, callee, err_mode));
#else
  (void)caller;
  (void)err_mode;
#endif

  IF_DEBUG({
    if (floor == ceil) {
      eprintf(
        "[system] skipped inlining " FMT_QUOTED ": zero useful instructions\n",
        callee->name.buf
      );
    }
    else {
      eprintf(
        "[system] inlined word " FMT_QUOTED "; instructions (" FMT_IND "):\n",
        callee->name.buf,
        ceil - floor
      );
      eprint_byte_range_hex((U8 *)&instrs->dat[floor], (U8 *)&instrs->dat[ceil]);
      fputc('\n', stderr);
    }
  });
  return nullptr;
}

static void asm_fixup_throw(Comp *comp, const Asm_fixup *fix, const Sym *sym) {
  IF_DEBUG(assert_fatal(fix->type == ASM_FIX_THROW));
  IF_DEBUG(assert_fatal(sym->type == SYM_NORM));

  const auto code  = &comp->code.code_write;
  const auto epi   = list_elem_ptr(code, sym->norm.spans.epi_err);
  const auto instr = fix->throw;
  const auto off   = epi - instr;

  IF_DEBUG(assert_fatal(*instr == asm_instr_breakpoint(ASM_CODE_THROW)));
  *instr = asm_instr_branch_to_offset(off);
}

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
      case ASM_FIX_THROW: {
        asm_fixup_throw(comp, fix, sym);
        continue;
      }
      case ASM_FIX_RECUR: {
        asm_fixup_recur(comp, fix, sym);
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
  const auto code  = &comp->code;
  const auto write = &code->code_write;
  const auto spans = &sym->norm.spans;

  if (sym->has_err) {
    const auto reg = asm_sym_err_reg(sym);
    if (reg >= 0) bits_add_to(&sym->clobber, (U8)reg);
  }

// TODO organize better, preferably by ripping out stack-CC.
#ifndef CALL_CONV_STACK
  asm_fixup_locals(comp, sym);
#endif

  asm_fixup_sym_prologue(comp, sym, &spans->prologue);
  asm_append_sym_epilogue(comp, sym);

  spans->ret = write->len;
  asm_append_instr(comp, ASM_INSTR_RET);

  spans->data = write->len;
  asm_fixup(comp, sym);

  // Execution never reaches this. Makes it easier to tell functions apart.
  spans->ceil = write->len;
  IF_DEBUG(asm_append_breakpoint(comp, ASM_CODE_PROC_DELIM));

  code->valid_instr_len = write->len;
  sym->norm.exec        = asm_sym_prologue_executable(comp, sym);
}
