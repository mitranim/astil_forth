#pragma once
#include "./err.c"
#include "./fmt.c"
#include "./io.c"
#include "./mem.c"
#include <spawn.h>
#include <stddefer.h>

static Err spawn_with_stdin(
  char *const argv[], const U8 *buf, Ind len, pid_t *pid
) {
  int pipes[2] = {-1, -1};
  try_errno(pipe(pipes));

  deferred(fd_deinit) int fd_read  = pipes[0];
  deferred(fd_deinit) int fd_write = pipes[1];

  posix_spawn_file_actions_t act;
  try_errno_posix(posix_spawn_file_actions_init(&act));
  defer posix_spawn_file_actions_destroy(&act);

  // Replace the child's stdin with our pipe.
  // Stdout and stderr are inherited by default.
  try_errno_posix(posix_spawn_file_actions_adddup2(&act, fd_read, STDIN_FILENO));

  // Closing the read pipe is optional.
  // Closing the write pipe in the child allows to deliver EOF.
  try_errno_posix(posix_spawn_file_actions_addclose(&act, fd_read));
  try_errno_posix(posix_spawn_file_actions_addclose(&act, fd_write));

  const auto               cmd  = argv[0];
  const posix_spawnattr_t *attr = nullptr;
  char *const             *env  = nullptr;

  const auto err = err_errno_posix(posix_spawnp(pid, cmd, &act, attr, argv, env));
  if (err) return err_wrapf("unable to spawn `%2$s`: %1$s", err, cmd);

  /*
  If the child closes stdin early, closing our read end prevents our own fd from
  masking the broken pipe. `write` then raises `SIGPIPE` by default, or returns
  `EPIPE` if `SIGPIPE` is ignored, blocked, or caught.
  */
  fd_deinit(&fd_read);

  try(write_all(fd_write, buf, len, nullptr));
  fd_deinit(&fd_write); // Deliver EOF to child process.
  return nullptr;
}

static Err wait_pid(pid_t pid, int *status) {
  for (;;) {
    int code;
    if (waitpid(pid, &code, 0) >= 0) {
      if (WIFSIGNALED(code)) {
        return errf("PID %d terminated with signal %d", pid, WTERMSIG(code));
      }

      *status = WEXITSTATUS(code);
      return nullptr;
    }

    code = errno;
    if (code == EINTR) continue;

    return errf(
      "unable to wait on PID %d; code: %d; msg: %s", pid, code, strerror(code)
    );
  }
}

static Err print_disasm(const void *src, Ind len) {
  const auto str_len = len * 2;
  const auto buf_cap = str_len + 1;

  /*
  `llvm-mc` doesn't seem to support disassembling actual
  raw bytes, so we have to convert them to hex first.
  */
  deferred(chars_deinit) char *buf = malloc(buf_cap);

  if (!buf) return errf("unable to allocate " FMT_IND " bytes", buf_cap);

  fmt_bytes_hex_into(buf, buf_cap, src, len);

  const auto  exe    = "llvm-mc";
  char *const argv[] = {exe, "--disassemble", "--hex", nullptr};
  pid_t       pid;
  int         status;

  try(spawn_with_stdin(argv, (U8 *)buf, str_len, &pid));
  try(wait_pid(pid, &status));

  if (status || DEBUG) {
    eprintf("[debug] %s exited with code %d\n", exe, status);
  }
  return nullptr;
}
