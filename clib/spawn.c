#pragma once
#include "./err.c"
#include "./fmt.c"
#include "./io.c"
#include "./mem.c"
#include <spawn.h>
#include <stddefer.h>

/*
Outdated implementation. Missing features:
- Normalize pipe descriptors above stderr before adding file actions.
- Use Darwin `POSIX_SPAWN_CLOEXEC_DEFAULT` and manually preserve stdio.
- Pass a valid null-terminated `envp` array instead of nil `envp` pointer.
*/
static Err spawn_stdin(char *const argv[], pid_t *pid, int *out_fd_write) {
  int pipes[2] = {-1, -1};
  try_errno(pipe(pipes));

  deferred(fd_deinit) int fd_read  = pipes[0];
  deferred(fd_deinit) int fd_write = pipes[1];

#ifdef __APPLE__
  // Darwin-specific. Report an early child-stdin close as
  // EPIPE instead of terminating the parent with SIGPIPE.
  try_errno(fcntl(fd_write, F_SETNOSIGPIPE, 1));
#endif

  posix_spawn_file_actions_t act;
  try_errno_posix(posix_spawn_file_actions_init(&act));
  defer posix_spawn_file_actions_destroy(&act);

  // Define child stdin as read end of our pipe.
  // Stdout and stderr are inherited by default.
  try_errno_posix(posix_spawn_file_actions_adddup2(&act, fd_read, STDIN_FILENO));

  // Closing `fd_read` in child leaves only child's stdin as a reference
  // to the read end of the pipe. If child also closes stdin prematurely,
  // parent writes receive `EPIPE` due to `F_SETNOSIGPIPE` on Darwin, or
  // `SIGPIPE` otherwise. Without this close, parent write can deadlock.
  //
  // Closing `fd_write` in child allows parent to deliver EOF
  // to child's `fd_read` by closing parent-side `fd_write`.
  try_errno_posix(posix_spawn_file_actions_addclose(&act, fd_read));
  try_errno_posix(posix_spawn_file_actions_addclose(&act, fd_write));

  const auto               cmd  = argv[0];
  const posix_spawnattr_t *attr = nullptr;
  char *const             *env  = nullptr;

  const auto err = err_errno_posix(posix_spawnp(pid, cmd, &act, attr, argv, env));
  if (err) return err_wrapf("unable to spawn `%2$s`: %1$s", err, cmd);

  /*
  If the child closes stdin early, closing our read end prevents our own FD
  from masking the broken pipe. F_SETNOSIGPIPE makes `write` return `EPIPE`
  rather than generating `SIGPIPE`.
  */
  fd_deinit(&fd_read);

  // Transfer ownership, skip deinit.
  *out_fd_write = fd_write;
  fd_write      = -1;
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

  deferred(fd_deinit) int fd_write = -1;

  try(spawn_stdin(argv, &pid, &fd_write));

  const auto write_err = err_wrapf(
    "unable to write subprocess stdin: %s",
    write_all(fd_write, (U8 *)buf, str_len, nullptr)
  );

  fd_deinit(&fd_write); // Deliver EOF.

  const auto wait_err = wait_pid(pid, &status);
  if (write_err) return write_err;
  if (wait_err) return wait_err;

  if (status || DEBUG) {
    eprintf("[debug] %s exited with code %d\n", exe, status);
  }
  return nullptr;
}
