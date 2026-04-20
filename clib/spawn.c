#pragma once
#include "./err.c"
#include "./fmt.c"
#include "./io.c"
#include <spawn.h>

static Err spawn_with_stdin(
  char *const argv[], const U8 *buf, Ind len, pid_t *pid
) {
  int pipes[2] = {-1, -1};
  try_errno_posix(pipe(pipes));

  const auto fd_read  = pipes[0];
  const auto fd_write = pipes[1];

  posix_spawn_file_actions_t act;
  auto err = err_errno_posix(posix_spawn_file_actions_init(&act));

  if (err) {
    close(fd_read);
    close(fd_write);
    return err;
  }

  // Replace the child's stdin with our pipe.
  // Stdout and stderr are inherited by default.
  posix_spawn_file_actions_adddup2(&act, fd_read, STDIN_FILENO);

  // Closing the read pipe is optional.
  // Closing the write pipe in the child allows to deliver EOF.
  posix_spawn_file_actions_addclose(&act, fd_read);
  posix_spawn_file_actions_addclose(&act, fd_write);

  const auto               cmd  = argv[0];
  const posix_spawnattr_t *attr = nullptr;
  char *const             *env  = nullptr;

  err = err_errno_posix(posix_spawnp(pid, cmd, &act, attr, argv, env));
  posix_spawn_file_actions_destroy(&act);

  // Parent doesn't need the read pipe.
  close(fd_read);

  if (err) {
    close(fd_write);
    return err_wrapf("unable to spawn `%2$s`: %1$s", err, cmd);
  }

  err = write_all(fd_write, buf, len, nullptr);
  close(fd_write); // Deliver EOF to child process.
  return err;
}

static Err wait_pid(pid_t pid, int *status) {
  for (;;) {
    int code = waitpid(pid, status, 0);

    if (!(code < 0)) break;

    code = errno;
    if (code == EINTR) continue;

    return errf(
      "unable to wait on PID %d; code: %d; msg: %s", pid, code, strerror(code)
    );
  }
  return nullptr;
}

static Err print_disasm(const void *src, Ind len) {
  const auto cap = len * 2;

  /*
  `llvm-mc` doesn't seem to support disassembling actual
  raw bytes, so we have to convert them to hex first.
  */
  defer(str_deinit) char *buf = malloc(cap);
  fmt_bytes_hex_into(buf, cap, src, len);

  const auto  proc   = "llvm-mc";
  char *const argv[] = {proc, "--disassemble", "--hex", nullptr};
  pid_t       pid;
  int         status;

  try(spawn_with_stdin(argv, (U8 *)buf, cap, &pid));
  try(wait_pid(pid, &status));

  if (status || DEBUG) {
    eprintf("[debug] %s exited with code %d\n", proc, status);
  }
  return nullptr;
}
