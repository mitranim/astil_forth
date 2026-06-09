#!/usr/bin/env python3

# BOT-GENERATED

import os
import pty
import select
import sys
import time


def read_until(fd, needle, timeout=5):
    out = b""
    end = time.monotonic() + timeout

    while time.monotonic() < end:
        ready, _, _ = select.select([fd], [], [], 0.05)
        if not ready:
            continue

        try:
            chunk = os.read(fd, 4096)
        except OSError:
            break

        if not chunk:
            break

        out += chunk
        if needle in out:
            return out

    raise AssertionError(f"missing {needle!r}; got:\n{out.decode(errors='replace')}")


def main():
    cmd = [sys.argv[1], "forth/lang.af", "-"]
    pid, master = pty.fork()

    if pid == 0:
        os.execv(cmd[0], cmd)

    try:
        read_until(master, b"Running Astil Forth REPL.")

        os.write(master, b".s\n")
        read_until(master, b"stack is empty")

        os.write(master, b".s\n")
        read_until(master, b"stack is empty")
    finally:
        try:
            os.kill(pid, 15)
        except ProcessLookupError:
            pass
        try:
            os.waitpid(pid, 0)
        except ChildProcessError:
            pass
        os.close(master)


if __name__ == "__main__":
    main()
