#! /usr/bin/env python3

# BOT-GENERATED

import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
CSRC = ROOT / "bench/scan_delims_simd.c"
ASRC = ROOT / "bench/scan_delims_simd.af"
CFLS = [
    ROOT / "bench/scan_delims_simd.c",
    ROOT / "bench/scan_delims_naive.c",
]
SHAPES = {
    ROOT / "bench/scan_delims_simd.af": ("65536 let: CAP", "4096  let: REPS", "CAP mem: BUF"),
    ROOT / "bench/scan_delims_naive.af": ("65536 let: CAP", "4096  let: REPS", "CAP mem: BUF"),
    ROOT / "bench/scan_delims.py": ("CAP = 1 << 16", "REPS =", "buf ="),
    ROOT / "bench/scan_delims.mjs": ("const CAP = 1 << 16", "const REPS =", "const buf ="),
    ROOT / "bench/scan_delims.lua": ("local CAP = 2 ^ 16", "local REPS =", "local buf ="),
    ROOT / "bench/scan_delims.lisp": ("(defconstant +cap+", "(defconstant +reps+", "(defparameter *buf*"),
}
PENV = {**os.environ, "SCAN_DELIMS_PRINT": "1"}


CMDS = [
    ("bench/scan_delims_simd.exe",),
    ("bench/scan_delims_naive.exe",),
    ("bench/scan_delims_simd_astil.exe",),
    ("./astil.exe", "bench/scan_delims_simd.af"),
    ("./astil.exe", "bench/scan_delims_naive.af"),
    ("bun", "run", "bench/scan_delims.mjs"),
    ("luajit", "bench/scan_delims.lua"),
    ("sbcl", "--script", "bench/scan_delims.lisp"),
    ("pypy3", "bench/scan_delims.py"),
    ("python3", "bench/scan_delims.py"),
]


def run(cmd):
    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        env=PENV,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode:
        print(proc.stdout, end="")
        print(proc.stderr, end="")
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    out = proc.stdout.strip()
    return out.splitlines()[-1] if out else ""


def main():
    if '__asm__ volatile("" ' in CSRC.read_text(encoding="utf-8"):
        print("empty asm barrier in C SIMD")
        return 1
    if "#include <arm_neon.h>" in CSRC.read_text(encoding="utf-8"):
        print("Arm-only NEON include in C SIMD")
        return 1
    if "0x" in ASRC.read_text(encoding="utf-8"):
        print("raw hex instruction in Astil SIMD")
        return 1
    for path in CFLS:
        src = path.read_text(encoding="utf-8")
        if "__asm__" in src:
            print("Arm-only asm in", path.name)
            return 1
        if '#include "../clib/num.h"' not in src or "FMT_UINT" not in src:
            print("missing num.h/FMT_UINT in", path.name)
            return 1
        for txt in ("#define CAP", "#define REPS", "buf[CAP]"):
            if txt not in src:
                print("missing", txt, "in", path.name)
                return 1
    for path, snippets in SHAPES.items():
        src = path.read_text(encoding="utf-8")
        for txt in snippets:
            if txt not in src:
                print("missing", txt, "in", path.name)
                return 1
    run(("make", "PROD=true", "build"))
    run(("make", "PROD=true", "bench/scan_delims_simd.exe"))
    run(("make", "PROD=true", "bench/scan_delims_naive.exe"))
    run(("./astil.exe", "bench/scan_delims_simd.af", "--build=bench/scan_delims_simd_astil.exe"))
    outs = [(cmd, run(cmd)) for cmd in CMDS]
    ref = outs[0][1]
    if not ref.isdecimal():
        print("expected numeric stdout in print mode")
        return 1
    for cmd, out in outs:
        if out != ref:
            print("expected:", ref)
            print("actual:  ", out)
            print("command: ", " ".join(cmd))
            return 1
    print("ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
