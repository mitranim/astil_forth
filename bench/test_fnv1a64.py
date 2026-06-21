# BOT-GENERATED

import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
WANT = "0x8d8a704cbb222325"


CASES = [
    ("astil_reg", ["./astil.exe", "bench/fnv1a64.af"]),
    ("astil_stack", ["./astil_s.exe", "bench/fnv1a64_s.af"]),
    ("clang", ["bench/fnv1a64.exe"]),
    ("gforth", ["gforth", "bench/fnv1a64_g.fs", "-e", "bye"]),
    ("luajit", ["luajit", "bench/fnv1a64.lua"]),
    ("bun", ["bun", "run", "bench/fnv1a64.mjs"]),
    ("sbcl", ["sbcl", "--script", "bench/fnv1a64.lisp"]),
    ("pypy3", ["pypy3", "bench/fnv1a64.py"]),
    ("python3", ["python3", "bench/fnv1a64.py"]),
]


def main() -> int:
    env = os.environ.copy()
    env["FNV1A64_PRINT"] = "1"

    subprocess.run(["make", "PROD=true", "build"], cwd=ROOT, check=True)
    subprocess.run(["make", "PROD=true", "bench/fnv1a64.exe"], cwd=ROOT, check=True)

    for name, cmd in CASES:
        proc = subprocess.run(cmd, cwd=ROOT, env=env, check=True, text=True, stdout=subprocess.PIPE)
        got = proc.stdout.strip().lower()
        if got != WANT:
            raise AssertionError(f"{name}: want {WANT}, got {got}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
