#include "./interp.c"
#include "./lib/cli.c"
#include "./lib/err.h"
#include "./lib/time.c"
#include "./mach_exc.c"
#include "./mach_o.c"
#include <stdio.h>
#include <string.h>

static void print_help(void) {
  fputs(
    "Astil Forth -- an experimental Forth system.\n"
    "Currently runs only on Apple Silicon. Usage:\n"
    "\n"
    "  astil <file0> <file1> ...\n"
    "\n"
    "For an interactive REPL session, pass `/dev/stdin` or `-`\n"
    "as a \"file name\", in any order. Note: you need to import\n"
    "`std:lang.af` first, in order to bootstrap the language.\n"
    "Hint: to exit the REPL, press Ctrl+D to terminate stdin.\n"
    "Hint: for a better experience, install and use `rlwrap`:\n"
    "\n"
    "  rlwrap astil std:lang.af -\n"
    "  rlwrap astil <file0> -\n"
    "  rlwrap astil <file0> <file1> -\n"
    "\n"
    "Flags:\n"
    "\n"
#ifndef CALL_CONV_STACK
    "  --build  -- AOT-compile a Mach-O executable\n"
#endif // CALL_CONV_STACK
    "  --debug  -- extremely verbose debug logging\n"
    "  --trace  -- enable C stack traces on errors\n"
    "  --timing -- print per-import execution time\n"
    "\n"
    "Env vars:\n"
    "\n"
    "  DEBUG  -- same as `--debug`\n"
    "  TRACE  -- same as `--trace`\n"
    "  TIMING -- same as `--timing`\n"
    "\n"
    "(Note: CLI args are order-sensitive.\n"
    "Every file is evaluated immediately.\n"
    "Flags take effect only when reached.)\n"
    "\n"
#ifndef CALL_CONV_STACK
    "Compiling to a native executable:\n"
    "\n"
    "  astil <file> --build=out.exe\n"
    "  ./out.exe\n"
    "\n"
#endif // CALL_CONV_STACK
    "Hint: an `std:*` import searches the following locations:\n"
    "- Directory `./forth` relative to the `astil` executable.\n"
    "- Directory `$HOME/.local/share/astil` (`make install`).\n",
    stderr
  );
}

static Err build(Interp *interp, const char *path) {
  if (!path || !path[0]) {
    return err_str("unable to build executable: missing output path");
  }

  const auto sym = dict_get(&interp->dict_exec, "main");
  if (!sym) {
    return err_str("unable to build executable: missing entry point `main`");
  }

  try(compile_mach_executable_to(interp, path, sym));
  return nullptr;
}

static Err main_run(int argc, const char *argv[]) {
  bool timing = false;
  try(env_bool("DEBUG", &DEBUG));
  try(env_bool("TRACE", &TRACE));
  try(env_bool("timing", &timing));

  Interp interp = {};
  try(interp_init(&interp));
  try(init_exception_handling());

  const auto ceil = argv + argc;

  while (++argv < ceil) {
    const char *key;
    const char *val;
    try(cli_key_val(*argv, &key, &val));

    if (!key && !val) break;

    if (!key) {
      Timing time = {.prefix = "[import] "};
      if (timing) timing_beg(&time);
      try(interp_import(&interp, val));
      if (timing) timing_end(&time);
      continue;
    }

    bool ok;

    try(cli_bool_for("--debug", key, val, &DEBUG, &ok));
    if (ok) continue;

    try(cli_bool_for("--trace", key, val, &TRACE, &ok));
    if (ok) continue;

    try(cli_bool_for("--timing", key, val, &timing, &ok));
    if (ok) continue;

    {
      bool help;
      try(cli_bool_for("--help", key, val, &help, &ok));
      if (ok) {
        if (help) print_help();
        continue;
      }
    }

#ifndef CALL_CONV_STACK
    if (!strcmp(key, "--build")) {
      try(build(&interp, val));
      continue;
    }
#endif // CALL_CONV_STACK

    return errf("unrecognized CLI flag " FMT_QUOTED, key);
  }

  return nullptr;
}

int main(int argc, const char *argv[]) {
  if (argc <= 1) {
    print_help();
    return 0;
  }

  const auto err = main_run(argc, argv);
  if (!err || err == ERR_QUIT) return 0;

  fprintf(stderr, "error: %s\n", err);
  if (TRACE) backtrace_print();
  if (DEBUG) abort();
  return 1;
}
