#pragma once
#include "./c_comp.c"
#include "./c_intrin.c"
#include "./c_read.c"

// clang-format off

/*
Used in unwinding and debugging.
TODO: could use a magic number.
*/
static bool interp_valid(const Interp *interp) {
  return (
    interp &&
    is_aligned(interp) &&
    is_aligned(&interp->comp) &&
    is_aligned(&interp->ints) &&
    is_aligned(&interp->syms) &&
    is_aligned(&interp->words) &&
    is_aligned(&interp->reader) &&
    is_aligned(&interp->snap) &&
    comp_valid(&interp->comp) &&
    reader_valid(interp->reader) &&
    stack_valid((const Stack *)&interp->ints) &&
    stack_valid((const Stack *)&interp->syms) &&
    dict_valid((const Dict *)&interp->words)
  );
}

// clang-format on

static Err interp_deinit(Interp *interp) {
  const auto syms = &interp->syms;

  for (stack_range(auto, sym, syms)) sym_deinit(sym);
  list_deinit(&interp->syms);
  dict_deinit(&interp->words);
  dict_deinit_with_keys((Dict *)&interp->imports);

  Err err = nullptr;
  err     = either(err, comp_deinit(&interp->comp));
  err     = either(err, stack_deinit(&interp->ints));
  err     = either(err, stack_deinit(&interp->syms));

  *interp = (Interp){};
  return err;
}

static Err interp_init_syms(Interp *interp) {
  const auto syms = &interp->syms;
  const auto comp = &interp->comp;

  for (auto intrin = INTRIN; intrin < arr_ceil(INTRIN); intrin++) {
    const auto sym = stack_push(syms, *intrin);
    sym->name.len  = (Ind)strlen(sym->name.buf);
    dict_set(&interp->words, sym->name.buf, sym);
    comp_register_dysym(comp, sym->name.buf, (U64)sym->intrin);
  }

  IF_DEBUG({
    const auto syms = &interp->words;
    aver(dict_has(syms, ":"));
    aver(dict_has(syms, ";"));

    const auto gots = &comp->code.gots;
    aver(gots->inds.len == arr_cap(INTRIN));
    aver(stack_len(&gots->names) == arr_cap(INTRIN));
  });
  return nullptr;
}

static Err interp_init(Interp *interp) {
  *interp       = (Interp){};
  Stack_opt opt = {.len = 1024};

  try(stack_init(&interp->syms, &opt));
  try(stack_init(&interp->ints, &opt));
  try(comp_init(&interp->comp));
  try(interp_init_syms(interp)); // Requires `comp_init` first.
  interp_snapshot(interp);

  IF_DEBUG({
    eprintf("[system] interpreter: %p\n", interp);
    eprintf("[system] integer stack floor: %p\n", interp->ints.floor);
    eprintf(
      "[system] instruction floor (writable): %p\n",
      interp->comp.code.code_write.dat
    );
    eprintf(
      "[system] instruction floor (executable): %p\n",
      interp->comp.code.code_exec.dat
    );
  });
  return nullptr;
}
