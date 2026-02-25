#include "./interp.c"
#include "./lib/cli.c"
#include "./lib/err.h"
#include "./lib/time.c"
#include <stdio.h>
#include <string.h>

// #include "./mach_exec.c"

#ifndef CALL_CONV_STACK
static Err init_exception_handling(void) { return nullptr; }
#else
#include "./mach_exc.c"
#endif

static void print_help(void) {
  eputs(
    "Astil Forth — an experimental Forth implementation.\n"
    "\n"
    "Usage:\n"
    "\n"
    "  astil <file0> <file1> ...\n"
    "\n"
    "For interactive REPL mode, specify \"/dev/stdin\" or \"-\":\n"
    "\n"
    "  astil -\n"
    "  astil <file0> -\n"
    "  astil <file0> <file1> -\n"
    "\n"
    "Hint: a program must first import \"std:lang.f\" to bootstrap the language.\n"
    "\n"
    "An \"std:\" import searches the following locations:\n"
    "- Directory \"./forth\" relative to the executable.\n"
    "- Directory \"$HOME/.local/share/astil\" (`make install`).\n"
    "\n"
    "Hint: for a better REPL experience, install and use `rlwrap`:\n"
    "\n"
    "  rlwrap astil std:lang.f -\n"
  );
}

static Err run(int argc, const char **argv) {
  bool timing = false;
  try(env_bool("TIMING", &timing));

  Interp interp = {};
  try(interp_init(&interp));
  try(init_exception_handling());

  for (int ind = 1; ind < argc; ind++) {
    Timing time = {.prefix = "[import] "};
    if (timing) timing_beg(&time);
    try(interp_import(&interp, argv[ind]));
    if (timing) timing_end(&time);
  }

  // try(compile_mach_executable(&interp));
  return nullptr;
}

int main(int argc, const char **argv) {
  if (argc <= 1) {
    print_help();
    return 0;
  }

  try_main(env_bool("DEBUG", &DEBUG));
  try_main(env_bool("TRACE", &TRACE));

  const auto err = run(argc, argv);
  if (!err || err == ERR_QUIT) return 0;

  fprintf(stderr, "error: %s\n", err);
  if (TRACE) backtrace_print();
  if (DEBUG) abort();
  return 1;
}

/*

regular compiler: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks hard -> machine code

regular interpreter: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks little -> VM code -> interpreter loop -> interpret VM code

regular JIT compiler: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks hard -> machine code -> runs

our JIT compiler: (C & minimal intrinsics) + program text ->
  compiler thinks little -> program runs and assembles -> machine code -> runs

*/
