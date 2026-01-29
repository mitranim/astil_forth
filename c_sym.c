#pragma once
#include "./c_sym.h"
#include "./lib/set.c"

static void sym_deinit(Sym *sym) {
  set_deinit(&sym->callees);
  set_deinit(&sym->callers);
}

static bool is_name_immediate(const Word_str *name) {
  return name && name->len &&
    (name->buf[0] == '#' || !strcmp(name->buf, "\\") ||
     !strcmp(name->buf, "(") || name->buf[name->len - 1] == '[' ||
     name->buf[0] == ']' || !strcmp(name->buf, ";"));
}

static bool is_sym_leaf(const Sym *sym) { return !sym->callees.len; }
