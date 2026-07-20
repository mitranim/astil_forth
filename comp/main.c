#include "../clib/cli.c"
#include "../clib/err.h"
#include "../clib/time.c"
#include "./interp.c"
#include "./mach_exc.c"
#include "./mach_o.c"
#include <stdio.h>
#include <string.h>

static void print_help() {
  fputs(
    "Astil Forth -- an experimental Forth system.\n"
    "Currently runs only on Apple Silicon. Usage:\n"
    "\n"
    "  astil <file0> <file1> ...\n"
    "\n"
    "For an interactive REPL session, use `/dev/stdin` or `-`\n"
    "as a \"file name\", in any order. Note: you need to import\n"
    "`lang.af` first thing; otherwise language doesn't exist.\n"
    "Hint: to exit the REPL, press Ctrl+D to terminate stdin.\n"
    "Hint: for a better experience, install and use `rlwrap`:\n"
    "\n"
    "  rlwrap astil lang.af -\n"
    "  rlwrap astil <file0> -\n"
    "  rlwrap astil <file0> <file1> -\n"
    "\n"
    "Flags:\n"
    "\n"
#ifndef CALL_CONV_STACK
    "  --build  -- AOT-compile a Mach-O executable\n"
    "  --slop   -- disable sloppy-code diagnostics\n"
#endif // CALL_CONV_STACK
    "  --debug  -- extremely verbose debug logging\n"
    "  --trace  -- enable C stack traces on errors\n"
    "  --timing -- print per-import execution time\n"
    "  --       -- stop interpreting CLI arguments\n"
    "\n"
    "Env vars:\n"
    "\n"
    "  DEBUG  -- same as `--debug`\n"
#ifndef CALL_CONV_STACK
    "  SLOP   -- same as `--slop`\n"
#endif // CALL_CONV_STACK
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
    "Hint: paths beginning with `/` or `.` are local.\n"
    "Other import paths search standard library locations:\n"
    "- Directory `forth` relative to the path of the executable.\n"
    "- Directory `$HOME/.local/share/astil` with `make install`.\n",
    stderr
  );
}

static Err build(Interp *interp, const char *path) {
  if (!path || !path[0]) {
    return err_str("unable to build executable: missing output path");
  }

  const auto sym = dict_get(&interp->dict_exec, ".main");
  if (!sym) {
    return err_str("unable to build executable: missing entry point `.main`");
  }

  try(compile_mach_executable_to(interp, path, sym));
  return nullptr;
}

static Err main_run(int argc, const char *argv[]) {
  bool timing = false;
  try(env_bool("DEBUG", &DEBUG));
  try(env_bool("TRACE", &TRACE));
  try(env_bool("timing", &timing));

  deferred(interp_deinit) Interp interp = {};
  try(interp_init(&interp));
  interp.argc = (Uint)argc;
  interp.argv = argv;

  try(env_bool("SLOP", &interp.slop));
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

      deferred(chars_deinit) char *path = realpath(val, nullptr);
      try(interp_import(&interp, path ? path : val));

      if (timing) timing_end(&time);
      continue;
    }

    if (!strcmp(key, "--eval")) {
      if (!val) continue;

      Timing time = {.prefix = "[eval] "};
      if (timing) timing_beg(&time);

      try(interp_eval(&interp, val));

      if (timing) timing_end(&time);
      continue;
    }

    // Conventional args terminator. The program is free to use the rest.
    if (!strcmp(key, "--")) break;

    bool ok;

    try(cli_bool_for("--debug", key, val, &DEBUG, &ok));
    if (ok) continue;

    try(cli_bool_for("--slop", key, val, &interp.slop, &ok));
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
      Timing time = {.prefix = "[build] "};
      if (timing) timing_beg(&time);

      try(build(&interp, val));

      if (timing) timing_end(&time);
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
