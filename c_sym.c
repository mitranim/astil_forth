#pragma once
#include "./c_sym.h"
#include "./lib/set.c"

static void sym_deinit(Sym *sym) {
  set_deinit(&sym->callees);
  set_deinit(&sym->callers);
}

/*
We have a convention where words starting or ending with some specific
characters are automatically considered immediate, without any opt-out.
Marking words as implicitly immediate is discouraged. Immediacy should
be apparent from word names.
*/
static bool is_name_immediate(const Word_str *name) {
  if (!name || !name->len) return false;

  const auto head = name->buf[0];
  if (head == '#') return true;

  const auto last = name->buf[name->len - 1];
  return last == ':' || last == '\'' || last == '"';
}

static bool is_sym_leaf(const Sym *sym) { return !sym->callees.len; }
