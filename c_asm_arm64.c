#pragma once
#include "./c_asm_arm64.h"
#include "./c_read.h"
#include "./c_sym.c"
#include "./lib/dict.c"
#include "./lib/err.c"
#include "./lib/fmt.c"
#include "./lib/jit.c"
#include "./lib/list.c"
#include "./lib/mem.c"
#include "./lib/misc.h"
#include "./lib/num.h"
#include "./lib/stack.c"
#include "./lib/str.c"
#include <assert.h>
#include <stdatomic.h>
#include <string.h>

/*
## Links

- Arm64 ISA in HTML format (say NO to horrible PDFs):
  - https://developer.arm.com/Architectures/A-Profile%20Architecture#Downloads
  - Look for `A64` in HTML format.
- Procedure call standard: https://github.com/ARM-software/abi-aa/blob/c51addc3dc03e73a016a1e4edf25440bcac76431/aapcs64/aapcs64.rst
- Condition codes' encoding: see documentation of `B.cond`.
- Condition codes' explanation: https://developer.arm.com/documentation/dui0801/l/Condition-Codes/Condition-code-suffixes-and-related-flags
  - Interchangeable: "CS" = "HS", "CC" = "LO".
- LLVM's arm64 codegen is defined in `.td` files here: https://github.com/llvm/llvm-project/tree/main/llvm/lib/Target/AArch64
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

## Extensions

Special registers:

- `x28` = interpreter pointer
- `x27` = top of integer stack
- `x26` = floor of integer stack
- `x0`  = error string pointer

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

/*
TODO: consider implementing exceptions entirely in Forth.

Seems to involve the following:
- Ability to wrap arbitrary words to give them unusual compilation semantics,
  in this case adding a "try" (error check with return). A word which "throws"
  is interpreted as normal (no weird additional behavior), but compiles with an
  added "try" check, which is appended to a "try" stack to be finalized by `;`.
- Ability to wrap `;` to retropatch "try" sites; they are not fully formed
  until `;`, because the offset is unknown. The sites use a scratch stack.
  The presence of at least one "try" site causes `;` to wrap the word with
  its own "try" site, propagating this further. `;` performs this detection
  and wrapping for all words.
- We'd also move function prologue and epilogue from C to `:` and `;`.
- Ability to differentiate compilation and execution semantics of words.
  (BTW don't need two spans; just make another symbol and take a pointer.)

Possible semantics combinations per word:

  interp    | compile                | note
  ----------|------------------------|---------------
  <code>    | compile call to <code> | normal
  forbidden | run <code>             | compile-only
  <code>    | run <code>             | immediate anywhere
  <code0>   | run <code1>            | weird, use cases exist
  <code>    | forbidden              | weird, use cases may not exist
*/

__asm__(".include \"./c_asm_arm64.s\"");

static Err asm_page_init_write(Asm_page **out) {
  const auto ptr = mem_map(sizeof(Asm_page), 0);
  if (ptr == MAP_FAILED) return err_mmap();

  const auto page = (Asm_page *)ptr;
  *out            = page;

  constexpr auto low  = sizeof(page->guard_low);
  constexpr auto high = offsetof(Asm_page, guard_high);
  static_assert(high > low);

  const auto beg = (U8 *)page + low;
  const auto len = high - low;
  return mprotect_mutable(beg, len);
}

static Err asm_page_init_exec(Asm_page **out) {
  const auto ptr = mem_map(sizeof(Asm_page), MAP_JIT);
  if (ptr == MAP_FAILED) return err_mmap();

  const auto page = (Asm_page *)ptr;
  *out            = page;

  try(mprotect_jit(page->instrs, sizeof(page->instrs)));
  try(mprotect_mutable(page->consts, sizeof(page->consts)));
  try(mprotect_mutable(page->datas, sizeof(page->datas)));
  try(mprotect_mutable(page->got, sizeof(page->got)));
  return nullptr;
}

static Err asm_page_deinit(Asm_page *val) {
  return !val ? nullptr : err_errno(munmap(val, sizeof(*val)));
}

static void asm_heap_init_lists(Asm_heap *heap) {
  const auto page = heap->page;

  heap->instrs.dat = page->instrs;
  heap->instrs.len = 0;
  heap->instrs.cap = sizeof(page->instrs);

  heap->consts.dat = page->consts;
  heap->consts.len = 0;
  heap->consts.cap = sizeof(page->consts);

  heap->datas.dat = page->datas;
  heap->datas.len = 0;
  heap->datas.cap = sizeof(page->datas);

  heap->got.dat = page->got;
  heap->got.len = 0;
  heap->got.cap = sizeof(page->got);
}

// clang-format off

// Sanity check used in segfault recovery.
static bool asm_heap_valid(const Asm_heap *val) {
  return (
    val &&
    is_aligned(val) &&
    is_aligned(&val->page) &&
    is_aligned(&val->instrs) &&
    is_aligned(&val->consts) &&
    is_aligned(&val->datas) &&
    is_aligned(&val->got) &&
    ((U64)val->page > (U64)UINT32_MAX + 1) &&
    list_valid((const List *)&val->instrs) &&
    list_valid((const List *)&val->consts) &&
    list_valid((const List *)&val->datas) &&
    list_valid((const List *)&val->got)
  );
}

// clang-format on

static Err asm_heap_sync(Asm *asm) {
  const auto exec  = &asm->exec;
  const auto write = &asm->write;
  const auto beg   = exec->instrs.dat;
  const auto len   = exec->instrs.cap;

  try(jit_before_write(beg, len));

  /*
  For the mutable data, the executable heap is the source of truth.
  This allows programs to update the values before final compilation.
  For everything else, the writable heap is the source of truth.
  */
  arr_copy(exec->page->instrs, write->page->instrs);
  arr_copy(exec->page->consts, write->page->consts);
  arr_copy(write->page->datas, exec->page->datas);
  arr_copy(exec->page->got, write->page->got);

  exec->instrs.len = write->instrs.len;
  exec->consts.len = write->consts.len;
  exec->datas.len  = write->datas.len;
  exec->got.len    = write->got.len;

  try(jit_after_write(beg, len));
  return nullptr;
}

static Err asm_deinit(Asm *asm) {
  list_deinit(&asm->fixup);
  dict_deinit(&asm->got.inds);
  Err err = nullptr;
  err     = either(err, asm_page_deinit(asm->write.page));
  err     = either(err, asm_page_deinit(asm->exec.page));
  err     = either(err, stack_deinit(&asm->got.names));
  *asm    = (Asm){};
  return err;
}

static Err asm_init(Asm *asm) {
  Stack_opt opt = {.len = arr_cap(asm->write.page->got)};
  try(stack_init(&asm->got.names, &opt));
  try(asm_page_init_write(&asm->write.page));
  try(asm_page_init_exec(&asm->exec.page));
  asm_heap_init_lists(&asm->write);
  asm_heap_init_lists(&asm->exec);
  return asm_heap_sync(asm);
}

// clang-format off

// Sanity check used in segfault recovery.
static bool asm_valid(const Asm *asm) {
  return (
    asm &&
    is_aligned(asm) &&
    is_aligned(&asm->write) &&
    is_aligned(&asm->exec) &&
    is_aligned(&asm->fixup) &&
    is_aligned(&asm->got) &&
    asm_heap_valid(&asm->write) &&
    asm_heap_valid(&asm->exec) &&
    list_valid((const List *)&asm->fixup) &&
    stack_valid((const Stack *)&asm->got.names) &&
    dict_valid((const Dict *)&asm->got.inds)
  );
}

// clang-format on

/*
Prog counter is the next instruction in `Asm.exec`.
The executable heap is outdated while assembling,
so we have to adjust it against the writable heap.
*/
static Instr *asm_next_prog_counter(const Asm *asm) {
  return list_spare_ptr(&asm->exec.instrs, asm->write.instrs.len);
}

static U8 *asm_next_const(const Asm *asm) {
  return list_spare_ptr(&asm->exec.consts, asm->write.consts.len);
}

static U8 *asm_next_data(const Asm *asm) {
  return list_spare_ptr(&asm->exec.datas, asm->write.datas.len);
}

static Instr *asm_next_writable_instr(const Asm *asm) {
  return list_next_ptr(&asm->write.instrs);
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
  if (reg >= 0 && reg < ASM_REG_LEN) return nullptr;
  return errf("invalid register value " FMT_SINT, reg);
}

// Defined in `./c_asm_arm64.s`.
extern Err asm_call_forth(Err err, void *fun, void *interp) __asm__(
  "asm_call_forth"
);

static Err asm_call_norm(const Reader *read, const Sym *sym, void *interp) {
  IF_DEBUG(eprintf(
    "[system] calling word " FMT_QUOTED
    " at instruction address %p (" READ_POS_FMT ")\n",
    sym->name.buf,
    sym->norm.exec.floor,
    READ_POS_ARGS(read)
  ));
  const auto err = asm_call_forth(nullptr, sym->norm.exec.floor, interp);
  IF_DEBUG(eprintf(
    "[system] done called word " FMT_QUOTED "; error: %p\n", sym->name.buf, err
  ));
  return err;
}

/*
We assume we're calling a C procedure, and we're doing that from C
which limits us to just 1 output, despite more supported by Arm64.
We could get around that by using an inline asm call and listing a
giant clobber list, but there's no point since `libc` doesn't have
any procedures with multiple outputs.
*/
static Err asm_call_extern_proc(Sint_stack *stack, const Sym *sym) {
  aver(sym->type == SYM_EXT_PROC);

  const auto fun     = (Extern_fun *)sym->ext_proc.fun;
  const auto inp_len = sym->ext_proc.inp_len;
  const auto out_len = sym->ext_proc.out_len;

  aver(inp_len <= ASM_PARAM_REG_LEN);
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

  IF_DEBUG({
    eprintf(
      "[system] calling external procedure " FMT_QUOTED
      " at address %p; inp_len: %d; out_len: %d\n",
      sym->name.buf,
      fun,
      inp_len,
      out_len
    );
  });

  const Sint out = fun(x0, x1, x2, x3, x4, x5, x6, x7);
  if (out_len) stack_push(stack, out);

  IF_DEBUG({
    eprintf(
      "[system] done called extern procedure " FMT_QUOTED "\n", sym->name.buf
    );
  });
  return nullptr;
}

/*
Variants:

  opc 00 = str
  opc 01 = ldr

  str <val_reg>, [<addr_reg>, <off>]
  ldr <val_reg>, [<addr_reg>, <off>]
*/
static void asm_append_load_store_offset(
  Asm *asm, bool is_load, U8 val_reg, U8 addr_reg, Uint off
) {
  aver(divisible_by(off, 8));
  off /= 8;

  averr(imm_unsigned(off, 12));
  averr(asm_validate_reg(val_reg));
  averr(asm_validate_reg(addr_reg));

  list_push(
    &asm->write.instrs,
    (Instr)0b11'111'0'01'00'000000000000'00000'00000 | ((Instr)is_load << 22) |
      ((Instr)off << 10) | ((Instr)addr_reg << 5) | val_reg
  );
}

static void asm_append_store_offset(Asm *asm, U8 src_reg, U8 addr_reg, Uint off) {
  asm_append_load_store_offset(asm, false, src_reg, addr_reg, off);
}

static void asm_append_load_offset(Asm *asm, U8 out_reg, U8 addr_reg, Uint off) {
  asm_append_load_store_offset(asm, true, out_reg, addr_reg, off);
}

/*
Variants:

  opc 00 = str
  opc 01 = ldr

  str val_reg, [addr_reg, -pre_mod]! // full descending
  str val_reg, [addr_reg], +post_mod // empty ascending

  ldr val_reg, [addr_reg], +post_mod // full descending
  ldr val_reg, [addr_reg, -pre_mod]! // empty ascending
*/
static void asm_append_load_store_modified(
  Asm *asm, bool is_load, U8 val_reg, U8 addr_reg, Sint mod
) {
  Instr order = mod < 0 ? 0b11 : 0b01;
  Instr mod_val;

  averr(imm_signed(mod, 9, &mod_val));
  averr(asm_validate_reg(val_reg));
  averr(asm_validate_reg(addr_reg));

  list_push(
    &asm->write.instrs,
    (Instr)0b11'111'0'00'00'0'000000000'00'00000'00000 | ((Instr)is_load << 22) |
      (mod_val << 12) | (order << 10) | ((Instr)addr_reg << 5) | val_reg
  );
}

static void asm_append_store_modified(
  Asm *asm, U8 src_reg, U8 addr_reg, Sint mod_val
) {
  asm_append_load_store_modified(asm, false, src_reg, addr_reg, mod_val);
}

static void asm_append_load_modified(
  Asm *asm, U8 out_reg, U8 addr_reg, Sint mod_val
) {
  asm_append_load_store_modified(asm, true, out_reg, addr_reg, mod_val);
}

static void asm_append_load_literal_offset(Asm *asm, U8 reg, Sint off) {
  averr(asm_validate_reg(reg));

  Instr imm;
  averr(imm_signed(off, 19, &imm));

  list_push(
    &asm->write.instrs,
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
static U16 asm_append_adrp(Asm *asm, U8 reg, Uint addr) {
  constexpr Uint bits      = 12;
  constexpr Uint mask      = (1 << bits) - 1;
  const auto     prog      = (Uint)(asm_next_prog_counter(asm));
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

  list_push(
    &asm->write.instrs,
    // adrp <reg>, <imm>
    (Instr)0b1'00'100'00'0000000000000000000'00000 | (low << 29) | (high << 5) |
      reg
  );
  return (U16)pageoff;
}

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
static Instr asm_instr_load_store_pair_modified(
  bool is_load, U8 reg0, U8 reg1, U8 addr_reg, S8 mod
) {
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

static void asm_append_load_store_pair_modified(
  Asm *asm, bool is_load, U8 reg0, U8 reg1, U8 addr_reg, S8 mod
) {
  list_push(
    &asm->write.instrs,
    asm_instr_load_store_pair_modified(is_load, reg0, reg1, addr_reg, mod);
  );
}

static void asm_append_store_pair_modified(
  Asm *asm, U8 reg0, U8 reg1, U8 addr_reg, S8 mod
) {
  asm_append_load_store_pair_modified(asm, 0b0, reg0, reg1, addr_reg, mod);
}

static void asm_append_load_pair_modified(
  Asm *asm, U8 reg0, U8 reg1, U8 addr_reg, S8 mod
) {
  asm_append_load_store_pair_modified(asm, 0b1, reg0, reg1, addr_reg, mod);
}

// The offset is PC-relative and implicitly times 4.
static Instr asm_instr_branch_to_offset(Sint off) {
  Instr imm;
  averr(imm_signed(off, 26, &imm));
  return (Instr)0b0'00'101'00000000000000000000000000 | imm;
}

static void asm_append_branch_to_offset(Asm *asm, Sint off) {
  list_push(&asm->write.instrs, asm_instr_branch_to_offset(off));
}

static void asm_append_branch_with_link_to_offset(Asm *asm, Sint off) {
  Instr imm;
  averr(imm_signed(off, 26, &imm));
  list_push(
    &asm->write.instrs, (Instr)0b1'00'101'00000000000000000000000000 | imm
  );
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

static void asm_append_compare_branch(Asm *asm, U8 reg, Sint off, bool non_zero) {
  list_push(&asm->write.instrs, asm_compare_branch(reg, off, non_zero));
}

static void asm_append_compare_branch_zero(Asm *asm, U8 reg, Sint off) {
  list_push(&asm->write.instrs, asm_compare_branch_zero(reg, off));
}

static void asm_append_compare_branch_non_zero(Asm *asm, U8 reg, Sint off) {
  list_push(&asm->write.instrs, asm_compare_branch_non_zero(reg, off));
}

static const Instr *asm_sym_epilogue_writable(const Asm *asm, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  return list_elem_ptr(&asm->write.instrs, sym->norm.spans.epilogue);
}

static const Instr *asm_sym_epilogue_executable(const Asm *asm, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  return list_elem_ptr(&asm->exec.instrs, sym->norm.spans.epilogue);
}

static void asm_fixup_ret(Asm *asm, const Asm_fixup *fixup, const Sym *sym) {
  aver(fixup->type == ASM_FIXUP_RET);

  const auto epi = asm_sym_epilogue_writable(asm, sym);
  const auto tar = fixup->ret.instr;
  const auto off = epi - tar;

  *tar = asm_instr_branch_to_offset(off);
}

static void asm_fixup_load(Asm *asm, const Asm_fixup *fixup) {
  aver(fixup->type == ASM_FIXUP_LOAD);

  const auto write = &asm->write;
  const auto load  = &fixup->load;
  const auto pci   = load->instr;
  const auto imm   = load->imm;
  const auto imm32 = (U32)imm;
  const auto top   = list_next_ptr(&write->instrs);

  Instr opc;
  if ((Uint)imm32 == imm) {
    opc = 0b00; // ldr <Wt>, <off>
    list_push(&write->instrs, imm32);
  }
  else {
    opc = 0b01; // ldr <Xt>, <off>
    list_push_raw_val(&write->instrs, imm);
  }

  Instr imm19;
  averr(imm_signed((top - pci), 19, &imm19));

  *pci = (Instr)0b00'011'0'00'0000000000000000000'00000 | (opc << 30) |
    (imm19 << 5) | load->reg;
}

static void asm_fixup_try(Asm *asm, const Asm_fixup *fixup, const Sym *sym) {
  aver(fixup->type == ASM_FIXUP_TRY);

  const auto epi = asm_sym_epilogue_writable(asm, sym);
  const auto off = epi - fixup->try;

  *fixup->try = asm_compare_branch_non_zero(ASM_ERR_REG, off);
}

static void asm_fixup(Asm *asm, const Sym *sym) {
  const auto src = &asm->fixup;

  for (Ind ind = 0; ind < src->len; ind++) {
    const auto fixup = &src->dat[ind];

    switch (fixup->type) {
      case ASM_FIXUP_RET: {
        asm_fixup_ret(asm, fixup, sym);
        continue;
      }
      case ASM_FIXUP_LOAD: {
        asm_fixup_load(asm, fixup);
        continue;
      }
      case ASM_FIXUP_TRY: {
        asm_fixup_try(asm, fixup, sym);
        continue;
      }
      default: unreachable();
    }
  }

  src->len = 0;
}

static void asm_append_add_imm(Asm *asm, U8 tar_reg, U8 src_reg, Sint imm) {
  Instr imm_val;
  averr(imm_signed(imm, 12, &imm_val));
  averr(asm_validate_reg(tar_reg));
  averr(asm_validate_reg(src_reg));

  list_push(
    &asm->write.instrs,
    (Instr)0b1'0'0'100010'0'000000000000'00000'00000 | (imm_val << 10) |
      ((Instr)src_reg << 5) | tar_reg
  );
}

static Instr *asm_append_breakpoint(Asm *asm, Instr imm) {
  averr(imm_unsigned(imm, 16));
  return list_push(
    &asm->write.instrs,
    // brk <imm>
    (Instr)0b110'101'00'001'0000000000000000'000'00 | (imm << 5)
  );
}

// Creates a frame record in accordance with arm64 ABI.
static void asm_append_sym_prologue(Asm *asm) {
  asm_append_store_pair_modified(asm, ASM_FP_REG, ASM_LINK_REG, ASM_SP_REG, -16);
  asm_append_add_imm(asm, ASM_FP_REG, ASM_SP_REG, 0);
}

static void asm_append_sym_epilogue(Asm *asm) {
  asm_append_load_pair_modified(asm, ASM_FP_REG, ASM_LINK_REG, ASM_SP_REG, 16);
}

static void asm_sym_beg(Asm *asm, Sym *sym) {
  asm->fixup.len = 0;

  const auto write = &asm->write;
  const auto spans = &sym->norm.spans;
  spans->prologue  = write->instrs.len;

  asm_append_sym_prologue(asm);
  spans->inner = write->instrs.len;
}

static void asm_sym_auto_inlinable(Sym *sym) {
  if (!is_sym_leaf(sym)) return;
  if (sym->throws) return;

  const auto spans = &sym->norm.spans;
  if (spans->data < spans->next) return;

  const auto len = spans->epilogue - spans->inner;
  if (len > 1) return;

  sym->inlined = true;

  IF_DEBUG(
    eprintf("[system] symbol " FMT_QUOTED " is auto-inlinable\n", sym->name.buf)
  );
}

// ret x30
static constexpr Instr
  ASM_INSTR_RET = 0b110'101'1'0'0'10'11111'0000'0'0'11110'00000;

static Err asm_sym_end(Asm *asm, Sym *sym) {
  const auto write = &asm->write;
  const auto exec  = &asm->exec;
  const auto spans = &sym->norm.spans;
  const auto code  = &sym->norm.exec;
  const auto leaf  = is_sym_leaf(sym);

  spans->epilogue = write->instrs.len;
  if (!leaf) asm_append_sym_epilogue(asm);

  spans->ret = write->instrs.len;
  list_push(&write->instrs, ASM_INSTR_RET);
  spans->data = write->instrs.len;
  asm_fixup(asm, sym);
  spans->next = write->instrs.len;

  // Execution never reaches this. Makes it easier to tell words apart.
  if (DEBUG) asm_append_breakpoint(asm, ASM_CODE_PROC_DELIM);

  try(asm_heap_sync(asm));
  code->ceil = list_next_ptr(&exec->instrs);
  code->top  = list_next_ptr(&exec->instrs);

  // See above; free to skip LR stash/restore for non-leaf procs.
  code->floor = list_elem_ptr(
    &exec->instrs, leaf ? spans->inner : spans->prologue
  );

  asm_sym_auto_inlinable(sym);
  return nullptr;
}

static void asm_append_ret(Asm *asm) {
  list_append(
    &asm->fixup,
    (Asm_fixup){
      .type      = ASM_FIXUP_RET,
      .ret.instr = asm_append_breakpoint(asm, ASM_CODE_RET_EPI),
    }
  );
}

static void asm_register_call(Asm *, const Sym *) {}

/*
TODO use or drop.

Must be invoked immediately after encoding branch-with-link instructions,
namely `bl` and `blr`. They update the LR (`x30`) by setting it to the next
program counter. Non-leaf procedures have a prologue which creates the frame
record `{x29, x30}` on the stack and updates `x29` to point to the new record.
When unwinding, we use frame records in conjunction with `&asm->cfi` to find
which words are being called.

  static void asm_register_call(Asm *asm, const Sym *caller) {
    map_set(&asm->cfi, asm_next_prog_counter(asm), caller);
  }
*/

static bool asm_is_instr_ours(const Asm *asm, const Instr *addr) {
  return is_list_elem(&asm->exec.instrs, addr);
}

static void asm_append_mov_reg_to_reg(Asm *asm, U8 tar_reg, U8 src_reg) {
  averr(asm_validate_reg(tar_reg));
  averr(asm_validate_reg(src_reg));

  list_push(
    &asm->write.instrs,
    (Instr)0b1'01'01010'00'0'00000'000000'11111'00000 | ((Instr)src_reg << 16) |
      (Instr)tar_reg
  );
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

static void asm_append_imm_to_reg(Asm *asm, U8 reg, Sint src) {
  averr(imm_unsigned(reg, 5));

  const auto instrs = &asm->write.instrs;
  const auto imm    = src < 0 ? ~(Uint)src : (Uint)src;

  // movn = 0b00
  // movz = 0b10
  const auto opc = (Instr)(src < 0 ? 0b00 : 0b10);

  // imm16 unsigned
  constexpr auto imm_max = ((Uint)1 << 16) - 1;

  const auto base = (Instr)0b1'00'100101'00'0000000000000000'00000 |
    (opc << 29) | reg;

  if (imm <= imm_max) {
    list_push(instrs, base | (Instr)((imm) << 5));
    return;
  }

  Instr out = 0;
  if ((out = asm_maybe_mov_imm_to_reg(base, 0b01, imm, imm_max, reg))) {
    list_push(instrs, out);
    return;
  }
  if ((out = asm_maybe_mov_imm_to_reg(base, 0b10, imm, imm_max, reg))) {
    list_push(instrs, out);
    return;
  }
  if ((out = asm_maybe_mov_imm_to_reg(base, 0b11, imm, imm_max, reg))) {
    list_push(instrs, out);
    return;
  }

  list_append(
    &asm->fixup,
    (Asm_fixup){
      .type       = ASM_FIXUP_LOAD,
      .load.instr = asm_append_breakpoint(asm, ASM_CODE_LOAD),
      .load.imm   = (Uint)src,
      .load.reg   = reg,
    }
  );
}

static void asm_append_stack_push_from(Asm *asm, U8 reg) {
  asm_append_store_modified(asm, reg, ASM_INT_TOP_REG, 8);
}

static void asm_append_stack_pop_into(Asm *asm, U8 reg) {
  asm_append_load_modified(asm, reg, ASM_INT_TOP_REG, -8);
}

static void asm_append_stack_push_imm(Asm *asm, Sint imm) {
  /*
  mov <reg>, <imm>
  str <reg>, [x27], 8
  */
  asm_append_imm_to_reg(asm, ASM_SCRATCH_REG_8, imm);
  asm_append_stack_push_from(asm, ASM_SCRATCH_REG_8);
}

static void asm_append_zero_reg(Asm *asm, U8 reg) {
  averr(asm_validate_reg(reg));
  const auto ireg  = (Instr)reg;
  const auto base  = (Instr)0b1'10'01010'00'0'00000'000000'00000'00000;
  const auto instr = base | (ireg << 16) | (ireg << 5) | ireg;
  list_push(&asm->write.instrs, instr);
}

static void asm_append_try(Asm *asm) {
  list_append(
    &asm->fixup,
    (Asm_fixup){
      .type = ASM_FIXUP_TRY,
      .try  = asm_append_breakpoint(asm, ASM_CODE_TRY),
    }
  );
}

/*
The instruction will contain a PC-relative offset of the instruction it's
going to "call". "PC" stands for "program counter": memory address of the
instruction we're about to encode.

Different CPU architectures have different notions of PC.
On arm64, it's the instruction currently executing.

The offset is implicitly times 4 at the CPU level.
*/
static void asm_append_branch_link(Asm *asm, Sint pc_off) {
  Instr imm26;
  averr(imm_signed(pc_off, 26, &imm26));

  // bl <imm>
  list_push(&asm->write.instrs, ((Instr)0b1'00'101 << 26) | imm26);
}

static void asm_append_call_norm(Asm *asm, Sym *caller, const Sym *callee) {
  const auto fun    = (Instr *)callee->norm.exec.floor;
  const auto pc_off = fun - asm_next_prog_counter(asm);

  aver(asm_is_instr_ours(asm, fun));
  asm_append_branch_link(asm, pc_off);
  asm_register_call(asm, caller);
  if (callee->throws) asm_append_try(asm);

  IF_DEBUG({
    eprintf(
      "[system] in " FMT_QUOTED ": appended call to " FMT_QUOTED
      " with executable address %p and PC offset " FMT_SINT
      " (in instruction count)\n",
      caller->name.buf,
      callee->name.buf,
      fun,
      pc_off
    );
  });
}

static void asm_append_branch_with_link_to_reg(Asm *asm, U8 reg) {
  averr(asm_validate_reg(reg));
  const auto base = (Instr)0b110'101'1'0'0'01'11111'0000'0'0'00000'00000;
  list_push(&asm->write.instrs, base | ((Instr)reg << 5));
}

static void asm_append_call_intrin_before(Asm *asm, Uint ints_top_off) {
  asm_append_store_offset(asm, ASM_INT_TOP_REG, ASM_INTERP_REG, ints_top_off);
}

static void asm_append_call_intrin_after(Asm *asm, Uint ints_top_off) {
  asm_append_load_offset(asm, ASM_INT_TOP_REG, ASM_INTERP_REG, ints_top_off);
}

/*
Used for registering dynamically-linked symbols.

The provided address should be used only in interpretation. When we implement
AOT compilation, GOT addresses should be pre-zeroed and patched by the linker.
*/
static Ind asm_register_dysym(Asm *asm, const char *name, U64 addr) {
  const auto names   = &asm->got.names;
  const auto inds    = &asm->got.inds;
  const auto val_ind = dict_ind(inds, name);

  if (ind_valid(val_ind)) return inds->vals[val_ind];

  const auto got     = &asm->write.got;
  const auto got_ind = got->len;

  list_push(got, addr);
  aver(stack_len(names) == got_ind);

  const auto got_name = names->top++;

  averr(str_copy(got_name, name));
  dict_set(inds, got_name->buf, got_ind);
  return got_ind;
}

static void asm_append_dysym_load(Asm *asm, const char *name, U8 reg) {
  const auto inds = &asm->got.inds;
  aver(dict_has(inds, name));

  const auto got_ind = dict_get(&asm->got.inds, name);
  aver(ind_valid(got_ind));

  const auto got_addr = asm->exec.page->got + got_ind;
  const auto pageoff  = asm_append_adrp(asm, reg, (Uint)got_addr);
  asm_append_load_offset(asm, reg, reg, pageoff);
}

/*
TODO: words which contain such calls may not appear in compiled executables.
We detect these calls and set `Sym.interp_only` for later use.
*/
static void asm_append_call_intrin(
  Asm *asm, const Sym *caller, const Sym *callee, Uint ints_top_off
) {
  aver(callee->type == SYM_INTRIN);

  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_call_intrin_before(asm, ints_top_off);
  asm_append_mov_reg_to_reg(asm, ASM_PARAM_REG_0, ASM_INTERP_REG);
  asm_append_dysym_load(asm, callee->name.buf, reg);
  asm_append_branch_with_link_to_reg(asm, reg);
  asm_register_call(asm, caller);
  asm_append_call_intrin_after(asm, ints_top_off);

  if (callee->throws) asm_append_try(asm);
  else asm_append_zero_reg(asm, ASM_ERR_REG);
}

static void asm_append_load_extern_ptr(Asm *asm, const char *name) {
  constexpr auto reg = ASM_SCRATCH_REG_8;
  asm_append_dysym_load(asm, name, reg);
  asm_append_stack_push_from(asm, reg);
}

static void asm_append_call_extern_proc(Asm *asm, Sym *caller, const Sym *callee) {
  aver(callee->type == SYM_EXT_PROC);

  const auto inp_len = callee->ext_proc.inp_len;
  const auto out_len = callee->ext_proc.out_len;

  aver(inp_len <= ASM_PARAM_REG_LEN);
  aver(out_len <= 1);

  // TODO use `ldp` for even pairs.
  if (inp_len > 7) asm_append_stack_pop_into(asm, ASM_PARAM_REG_7);
  if (inp_len > 6) asm_append_stack_pop_into(asm, ASM_PARAM_REG_6);
  if (inp_len > 5) asm_append_stack_pop_into(asm, ASM_PARAM_REG_5);
  if (inp_len > 4) asm_append_stack_pop_into(asm, ASM_PARAM_REG_4);
  if (inp_len > 3) asm_append_stack_pop_into(asm, ASM_PARAM_REG_3);
  if (inp_len > 2) asm_append_stack_pop_into(asm, ASM_PARAM_REG_2);
  if (inp_len > 1) asm_append_stack_pop_into(asm, ASM_PARAM_REG_1);
  if (inp_len > 0) asm_append_stack_pop_into(asm, ASM_PARAM_REG_0);

  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_dysym_load(asm, callee->name.buf, reg);
  asm_append_branch_with_link_to_reg(asm, reg);
  asm_register_call(asm, caller);
  if (out_len) asm_append_stack_push_from(asm, ASM_ERR_REG);
  asm_append_zero_reg(asm, ASM_ERR_REG);
}

static Err asm_append_instr_from_int(Asm *asm, Sint val) {
  try(imm_unsigned((Uint)val, sizeof(Instr) * CHAR_BIT));
  list_push(&asm->write.instrs, (Instr)val);
  return nullptr;
}

static Err err_out_of_space_const() {
  return err_str("unable to allocate a constant: out of space");
}

static Err asm_alloc_const_append_load(Asm *asm, U8 const *src, Uint len, U8 reg) {
  const auto tar = &asm->write.consts;
  if (len > list_rem_bytes(tar)) return err_out_of_space_const();

  const auto addr = asm_next_const(asm);
  list_push_raw(tar, src, len);

  const auto pageoff = asm_append_adrp(asm, reg, (Uint)addr);
  if (pageoff) asm_append_add_imm(asm, reg, reg, pageoff);
  return nullptr;
}

static Err err_out_of_space_data() {
  return err_str("unable to allocate static data: out of space");
}

static Err asm_alloc_data_append_load(Asm *asm, Uint len, U8 reg, U8 **out_addr) {
  const auto tar = &asm->write.datas;
  if (len > list_rem_bytes(tar)) return err_out_of_space_data();

  const auto addr = asm_next_data(asm);
  tar->len += len * list_val_size(tar);

  const auto pageoff = asm_append_adrp(asm, reg, (Uint)addr);
  if (pageoff) asm_append_add_imm(asm, reg, reg, pageoff);
  if (out_addr) *out_addr = addr;
  return nullptr;
}

static Err err_inline_has_data(const Sym *sym) {
  return errf(
    "unable to inline word " FMT_QUOTED ": loads local immediate values",
    sym->name.buf
  );
}

/*
Simple, naive inlining without support for relocation.
Used manually in Forth code via an intrinsic.
*/
static Err asm_inline_sym(Asm *asm, const Sym *sym) {
  aver(sym->type == SYM_NORM);
  aver(is_sym_leaf(sym));

  const auto spans = &sym->norm.spans;
  if (spans->data < spans->next) return err_inline_has_data(sym);

  const auto instrs = &asm->write.instrs;
  const auto floor  = spans->inner;
  const auto ceil   = spans->ret;

  for (Ind ind = floor; ind < ceil; ind++) {
    list_push(instrs, instrs->dat[ind]);
  }

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
