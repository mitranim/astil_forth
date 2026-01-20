#pragma once
#include "./c_comp.h"
#include "./c_sym.c"
#include "./lib/dict.c"
#include "./lib/list.c"
#include "./lib/stack.c"
#include "./lib/str.c"

#ifdef NATIVE_CALL_ABI
#include "./c_comp_cc_reg.c"
#else
#include "./c_comp_cc_stack.c"
#endif

static bool instr_heap_valid(const Instr_heap *val) {
  return val && is_aligned(val) && is_aligned(&val->instrs);
}

// clang-format off

// Sanity check used in segfault recovery.
static bool comp_heap_valid(const Comp_heap *val) {
  return (
    val &&
    is_aligned(val) &&
    is_aligned(&val->exec) &&
    instr_heap_valid(&val->exec) &&
    is_aligned(&val->got)
  );
}

// Sanity check used in segfault recovery.
static bool comp_code_valid(const Comp_code *code) {
  return (
    code &&
    is_aligned(code) &&
    is_aligned(&code->write) &&
    is_aligned(&code->heap) &&
    is_aligned(&code->code_write) &&
    is_aligned(&code->code_exec) &&
    is_aligned(&code->consts) &&
    is_aligned(&code->data) &&
    is_aligned(&code->got) &&
    is_aligned(&code->gots) &&
    is_aligned(&code->valid_instr_len) &&
    instr_heap_valid(code->write) &&
    comp_heap_valid(code->heap) &&
    list_valid((const List *)&code->code_write) &&
    list_valid((const List *)&code->code_exec) &&
    list_valid((const List *)&code->consts) &&
    list_valid((const List *)&code->data) &&
    list_valid((const List *)&code->got) &&
    (
      stack_valid((const Stack *)&code->gots.names) &&
      dict_valid((const Dict *)&code->gots.inds)
    )
  );
}

// Sanity check used in unwinding and debugging.
static bool comp_valid(const Comp *comp) {
  return (
    comp &&
    is_aligned(comp) &&
    is_aligned(&comp->code) &&
    is_aligned(&comp->ctx) &&
    comp_code_valid(&comp->code) &&
    comp_ctx_valid(&comp->ctx)
  );
}

// clang-format on

static Err instr_heap_deinit(Instr_heap **out) {
  if (!out || !*out) return nullptr;
  try(err_errno(munmap(*out, sizeof(**out))));
  *out = nullptr;
  return nullptr;
}

static Err instr_heap_init(Instr_heap **out) {
  const auto ptr = mem_map(sizeof(Instr_heap), 0);
  if (ptr == MAP_FAILED) return err_mmap();

  const auto base = (Instr_heap *)ptr;
  *out            = base;
  return mprotect_mutable(base->instrs, sizeof(base->instrs));
}

static Err comp_heap_deinit(Comp_heap **out) {
  if (!out || !*out) return nullptr;
  try(err_errno(munmap(*out, sizeof(**out))));
  *out = nullptr;
  return nullptr;
}

static Err comp_heap_init(Comp_heap **out) {
  const auto ptr = mem_map(sizeof(Comp_heap), MAP_JIT);
  if (ptr == MAP_FAILED) return err_mmap();

  const auto heap = (Comp_heap *)ptr;
  *out            = heap;

  try(mprotect_jit(heap->exec.instrs, sizeof(heap->exec.instrs)));
  try(mprotect_mutable(heap->consts, sizeof(heap->consts)));
  try(mprotect_mutable(heap->data, sizeof(heap->data)));
  try(mprotect_mutable(heap->got, sizeof(heap->got)));
  return nullptr;
}

static Err comp_code_deinit(Comp_code *code) {
  dict_deinit(&code->gots.inds);
  Err err = stack_deinit(&code->gots.names);
  err     = either(err, instr_heap_deinit(&code->write));
  err     = either(err, comp_heap_deinit(&code->heap));
  *code   = (Comp_code){};
  return err;
}

static void comp_code_init_lists(Comp_code *code) {
  code->code_write = (typeof(code->code_write)){
    .dat = code->write->instrs,
    .len = 0,
    .cap = arr_cap(code->write->instrs),
  };

  code->code_exec = (typeof(code->code_exec)){
    .dat = code->heap->exec.instrs,
    .len = 0,
    .cap = arr_cap(code->heap->exec.instrs),
  };

  code->consts = (typeof(code->consts)){
    .dat = code->heap->consts,
    .len = 0,
    .cap = arr_cap(code->heap->consts),
  };

  code->data = (typeof(code->data)){
    .dat = code->heap->data,
    .len = 0,
    .cap = arr_cap(code->heap->data),
  };

  code->got = (typeof(code->got)){
    .dat = code->heap->got,
    .len = 0,
    .cap = arr_cap(code->heap->got),
  };
}

static Err comp_code_init(Comp_code *code) {
  *code = (Comp_code){};

  try(instr_heap_init(&code->write));
  try(comp_heap_init(&code->heap));
  comp_code_init_lists(code);

  Stack_opt opt = {.len = arr_cap(code->heap->got)};
  try(stack_init(&code->gots.names, &opt));
  return nullptr;
}

static Err comp_init(Comp *comp) {
  try(comp_code_init(&comp->code));
  try(comp_ctx_init(&comp->ctx));
  return nullptr;
}

static Err comp_deinit(Comp *comp) {
  try(comp_ctx_deinit(&comp->ctx));
  try(comp_code_deinit(&comp->code));
  return nullptr;
}

static Comp_ctx comp_ctx_rewind(Comp_ctx prev, Comp_ctx next) {
  stack_rewind(&next.locals, &prev.locals);
  dict_rewind(&next.local_dict, &prev.local_dict);
  return next;
}

static Comp comp_rewind(Comp prev, Comp next) {
  next.ctx = comp_ctx_rewind(prev.ctx, next.ctx);
  return next;
}

/*
Used for registering dynamically-linked symbols.

The provided address should be used only in interpretation. When we implement
AOT compilation, GOT addresses should be pre-zeroed and patched by the linker.
*/
static Ind comp_register_dysym(Comp *comp, const char *name, U64 addr) {
  const auto code    = &comp->code;
  const auto names   = &code->gots.names;
  const auto inds    = &code->gots.inds;
  const auto val_ind = dict_ind(inds, name);

  if (ind_valid(val_ind)) return inds->vals[val_ind];

  const auto got     = &code->got;
  const auto got_ind = got->len;

  list_push(got, addr);
  aver(stack_len(names) == got_ind);

  const auto got_name = names->top++;

  averr(str_copy(got_name, name));
  dict_set(inds, got_name->buf, got_ind);
  return got_ind;
}

static Err err_current_sym_not_defining() {
  return err_str("unable to find current word: not inside a colon definition");
}

static Err comp_require_current_sym(const Comp *comp, Sym **out) {
  const auto sym = comp->ctx.sym;
  if (!sym) return err_current_sym_not_defining();
  if (out) *out = sym;
  return nullptr;
}

static Err err_internal_dup_local(const char *name) {
  return errf("internal error: duplicate local name " FMT_QUOTED, name);
}

static Err comp_local_add(Comp *comp, Word_str word, Local **out) {
  const auto stack = &comp->ctx.locals;
  const auto dict  = &comp->ctx.local_dict;
  const auto name  = word.buf;
  if (dict_has(dict, name)) return err_internal_dup_local(name);

  const auto loc = stack_push(stack, (Local){.name = word});
  dict_set(dict, loc->name.buf, loc);
  if (out) *out = loc;
  return nullptr;
}

static Local *comp_local_get(Comp *comp, const char *name) {
  return dict_get(&comp->ctx.local_dict, name);
}

static Err comp_local_get_or_make(Comp *comp, Word_str name, Local **out) {
  auto loc = comp_local_get(comp, name.buf);
  if (loc) {
    if (out) *out = loc;
    return nullptr;
  }
  return comp_local_add(comp, name, out);
}

static Local *comp_local_anon(Comp *comp) {
  return stack_push(&comp->ctx.locals, (Local){});
}

static Err err_local_invalid(Local *ptr) {
  return errf("%p is not a valid address of a local", ptr);
}

static Err comp_validate_local(Comp *comp, Sint num, Local **out) {
  const auto locs = &comp->ctx.locals;
  const auto ptr  = (Local *)num;
  if (!is_stack_elem(locs, ptr)) return err_local_invalid(ptr);
  if (out) *out = ptr;
  return nullptr;
}

static Err err_inline_not_defining() {
  return err_str("unsupported use of \"inline\" outside a colon definition");
}

static Err comp_inline_sym(Comp *comp, Sym *sym) {
  if (!comp->ctx.sym) return err_inline_not_defining();
  return asm_inline_sym(comp, sym);
}

static Err comp_append_call_sym(Comp *comp, Sym *callee) {
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  sym_auto_comp_only(caller, callee);
  sym_auto_interp_only(caller, callee);

  if (callee->norm.inlinable) return comp_inline_sym(comp, callee);

  set_add(&caller->callees, callee);
  set_add(&callee->callers, caller);
  sym_auto_throws(caller, callee);

  switch (callee->type) {
    case SYM_NORM: {
      comp_append_call_norm(comp, caller, callee);
      break;
    }
    case SYM_INTRIN: {
      asm_append_call_intrin(comp, caller, callee, INTERP_INTS_TOP);
      break;
    }
    case SYM_EXT_PTR: {
      asm_append_load_extern_ptr(comp, callee->name.buf);
      break;
    }
    case SYM_EXT_PROC: {
      asm_append_call_extern_proc(comp, caller, callee);
      break;
    }
    default: unreachable();
  }

  IF_DEBUG(eprintf(
    "[system] compiled call of symbol " FMT_QUOTED "\n", callee->name.buf
  ));
  return nullptr;
}

static void comp_sym_beg(Comp *comp, Sym *sym) {
  comp_ctx_trunc(&comp->ctx);
  comp->ctx.sym       = sym;
  comp->ctx.compiling = true;
  asm_sym_beg(comp, sym);
}

static void comp_sym_end(Comp *comp, Sym *sym) {
  asm_sym_end(comp, sym);
  sym_auto_inlinable(sym);
  comp_ctx_trunc(&comp->ctx);
}

static void comp_debug_print_sym_instrs(
  const Comp *comp, const Sym *sym, const char *prefix
) {
  const auto code      = &comp->code;
  const auto write     = &code->code_write;
  const auto spans     = &sym->norm.spans;
  const auto write_beg = &write->dat[spans->begin];
  const auto write_end = &write->dat[spans->next];

  eprintf(
    "%swritable code address: %p\n"
    "%sinstructions in writable code heap (" FMT_SINT "):\n",
    prefix,
    write_beg,
    prefix,
    write_end - write_beg
  );
  fputs(prefix, stderr);
  eprint_byte_range_hex((U8 *)write_beg, (U8 *)write_end);
  fputc('\n', stderr);

  const auto exec     = &sym->norm.exec;
  const auto exec_beg = exec->floor;
  const auto exec_end = exec->top;

  eprintf(
    "%sexecutable code address: %p\n"
    "%sinstructions in executable code heap (" FMT_SINT "):\n",
    prefix,
    exec_beg,
    prefix,
    exec_end - exec_beg
  );
  fputs(prefix, stderr);
  eprint_byte_range_hex((U8 *)exec_beg, (U8 *)exec_end);
  fputc('\n', stderr);
}

static void comp_append_ret(Comp *comp) {
  list_append(
    &comp->ctx.fixup,
    (Comp_fixup){
      .type = COMP_FIX_RET,
      .ret  = asm_append_breakpoint(comp, ASM_CODE_RET),
    }
  );
}

static void comp_append_recur(Comp *comp) {
  list_append(
    &comp->ctx.fixup,
    (Comp_fixup){
      .type  = COMP_FIX_RECUR,
      .recur = asm_append_breakpoint(comp, ASM_CODE_RECUR),
    }
  );
}
