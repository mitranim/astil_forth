#! /usr/bin/env python3

"""
Driver for our cross-language benchmarks.

User-facing wall time is primary. Total CPU exposes compute and power waste
hidden by wall time; peak mem/RSS is useful evidence, but not a claim about
live or strictly required memory, especially for GC runtimes.

Had to replace `hyperfine` with an inline benchmark runner
in order to collect per-process CPU and mem/RSS statistics
via `wait4`; `hyperfine` failed to provide them.

Subprocesses are invoked directly and inherit stdio.
Benchmarks tests their results internally, and report
errors via stderr + exit code. Working benchmarks are
supposed to be silent.

Before measurement, we run every benchmark once to validate them,
and possibly warm up the disk cache.

All benchmarks in each family must do identical work,
producing an identical result (validated internally).

Measurement stops when the wall-time mean is precise enough,
or when reaching the hard one-minute limit.
"""

# BOT-GENERATED

import argparse
import fnmatch
import math
import os
import signal
import statistics
import subprocess
import sys
import time
from contextlib import contextmanager, nullcontext
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
GEN = ROOT / "generated"
DEFAULT_OUTPUT = GEN / "bench.md"

MIN_RUNS = 5
MIN_MEASURE_SECONDS = 1.0
MAX_MEASURE_SECONDS = 60.0
MAX_RELATIVE_STANDARD_ERROR = 0.01
PROGRESS_SECONDS = 1.0
MAX_IQR_RATIO = 0.10
# Darwin reports ru_maxrss in bytes; Linux and the other supported Unixes use KiB.
RSS_UNIT_BYTES = 1 if sys.platform == "darwin" else 1024

CLEAN = ("make", "clean")
BUILD = ("make", "PROD=true", "build")

TOOLS = {
    "clang": ("clang", "--version"),
    "go": ("go", "version"),
    "zig": ("zig", "version"),
    "java": ("java", "-version"),
    "gforth": ("gforth", "--version"),
    "luajit": ("luajit", "-v"),
    "bun": ("bun", "--version"),
    "sbcl": ("sbcl", "--version"),
    "pypy3": ("pypy3", "--version"),
    "python3": ("python3", "--version"),
}


@dataclass(frozen=True)
class Bench:
    section: str
    name: str
    file: str
    cmd: tuple[str, ...]
    setup: tuple[tuple[str, ...], ...] = ()
    tools: tuple[str, ...] = ()


@dataclass(frozen=True)
class Sample:
    wall_seconds: float
    cpu_seconds: float
    peak_rss_bytes: int


@dataclass(frozen=True)
class Result:
    item: Bench
    samples: list[Sample]


BENCHES: list[Bench] = []
SECTIONS: list[str] = []


def section(title: str) -> None:
    SECTIONS.append(title)


def bench(
    name: str,
    file: str,
    cmd: tuple[str, ...],
    *,
    setup: tuple[tuple[str, ...], ...] = (),
    tools: tuple[str, ...] = (),
) -> None:
    BENCHES.append(Bench(SECTIONS[-1], name, file, cmd, setup, tools))


def wall_instability(samples: list[Sample]) -> float | None:
    if len(samples) < 2:
        return None
    walls = [sample.wall_seconds for sample in samples]
    q1, _, q3 = statistics.quantiles(walls, n=4, method="inclusive")
    ratio = (q3 - q1) / statistics.median(walls)
    return ratio if ratio > MAX_IQR_RATIO else None


def measurement_stable(samples: list[Sample]) -> bool:
    """Whether completed wall samples estimate their mean precisely enough."""
    if len(samples) < MIN_RUNS:
        return False
    walls = [sample.wall_seconds for sample in samples]
    if sum(walls) < MIN_MEASURE_SECONDS:
        return False
    relative_standard_error = (
        statistics.stdev(walls)
        / math.sqrt(len(walls))
        / statistics.mean(walls)
    )
    return relative_standard_error <= MAX_RELATIVE_STANDARD_ERROR


class MeasurementDeadline(BaseException):
    """Escape generic wait-error wrapping so the caller can discard this run."""


def deadline_expired(_signum, _frame) -> None:
    raise MeasurementDeadline


@contextmanager
def alarm(seconds: float):
    """Interrupt one owned-child wait without polling or a helper thread."""
    previous = signal.signal(signal.SIGALRM, deadline_expired)
    signal.setitimer(signal.ITIMER_REAL, seconds)
    try:
        yield
    finally:
        signal.setitimer(signal.ITIMER_REAL, 0)
        signal.signal(signal.SIGALRM, previous)


def terminate_and_reap(pid: int) -> None:
    # Probe ownership before signaling: wait4 may already have reaped the child.
    try:
        while True:
            try:
                waited, _ = os.waitpid(pid, os.WNOHANG)
                break
            except InterruptedError:
                continue
    except OSError:
        return
    if waited:
        return
    try:
        os.kill(pid, signal.SIGKILL)
    except OSError:
        pass
    try:
        while True:
            try:
                os.waitpid(pid, 0)
                break
            except InterruptedError:
                continue
    except OSError:
        pass


def measure(
    item: Bench, timeout_seconds: float | None = None
) -> Sample:
    cmd = item.cmd
    started = time.perf_counter_ns()
    if timeout_seconds is not None and timeout_seconds <= 0:
        raise MeasurementDeadline
    try:
        pid = os.posix_spawnp(cmd[0], cmd, os.environ)
    except OSError as err:
        raise RuntimeError(
            f"benchmark {item.name!r} failed to spawn command: {cmd!r}"
        ) from err
    if timeout_seconds is not None:
        timeout_seconds -= (
            time.perf_counter_ns() - started
        ) / 1_000_000_000
        if timeout_seconds <= 0:
            terminate_and_reap(pid)
            raise MeasurementDeadline
    try:
        alarm_context = (
            alarm(timeout_seconds)
            if timeout_seconds is not None
            else nullcontext()
        )
        with alarm_context:
            while True:
                try:
                    _, status, usage = os.wait4(pid, 0)
                    break
                except InterruptedError:
                    continue
    except BaseException as err:
        # ECHILD means ownership is gone; signaling this PID could hit its reuse.
        if not isinstance(err, ChildProcessError):
            terminate_and_reap(pid)
        if not isinstance(err, Exception):
            raise
        raise RuntimeError(
            f"benchmark {item.name!r} failed while waiting for command: "
            f"{cmd!r}"
        ) from err
    wall_seconds = (time.perf_counter_ns() - started) / 1_000_000_000
    exit_code = os.waitstatus_to_exitcode(status)
    if exit_code:
        raise RuntimeError(
            f"benchmark {item.name!r} command exited with exit status "
            f"{exit_code}: {cmd!r}"
        )
    return Sample(
        wall_seconds,
        usage.ru_utime + usage.ru_stime,
        usage.ru_maxrss * RSS_UNIT_BYTES,
    )


def c_exe(exe: str) -> tuple[tuple[str, ...], ...]:
    return (("make", "PROD=true", exe),)


def go_exe(src: str, exe: str) -> tuple[tuple[str, ...], ...]:
    return (("go", "build", "-o", exe, f"./{src}"),)


def zig_exe(src: str, exe: str) -> tuple[tuple[str, ...], ...]:
    return (("zig", "build-exe", "-O", "ReleaseFast", f"-femit-bin={exe}", src),)


def java_class(src: str) -> tuple[tuple[str, ...], ...]:
    return (("javac", src),)


def aot(src: str, exe: str) -> tuple[tuple[str, ...], ...]:
    return (BUILD, ("./astil.exe", src, f"--build={exe}"))


section("NONE")
bench("none_astil_reg", "astil.exe", ("./astil.exe", "--eval="), setup=(BUILD,), tools=("clang",))
bench("none_astil_stack", "astil_s.exe", ("./astil_s.exe", "--eval="), setup=(BUILD,), tools=("clang",))

section("BASELINE")
bench("baseline_astil_reg", "forth/lang.af", ("./astil.exe", "forth/lang.af"), setup=(BUILD,), tools=("clang",))
bench("baseline_astil_stack", "forth/lang_s.af", ("./astil_s.exe", "forth/lang_s.af"), setup=(BUILD,), tools=("clang",))
bench("baseline_gforth", "gforth", ("gforth", "-e", "bye"), tools=("gforth",))
bench("baseline_luajit", "luajit", ("luajit", "-e", ""), tools=("luajit",))
bench("baseline_js_bun", "bun", ("bun", "-e", ";"), tools=("bun",))
bench("baseline_cl_sbcl", "sbcl", ("sbcl", "--noinform", "--non-interactive"), tools=("sbcl",))
bench("baseline_pypy", "pypy3", ("pypy3", "-c", ""), tools=("pypy3",))
bench("baseline_python", "python3", ("python3", "-c", ""), tools=("python3",))

section("BUBBLE")
bench("bubble_clang", "bench/bubble.c", ("bench/bubble.exe",), setup=c_exe("bench/bubble.exe"), tools=("clang",))
bench("bubble_astil_aot", "bench/bubble.af", ("bench/bubble_astil.exe",), setup=aot("bench/bubble.af", "bench/bubble_astil.exe"), tools=("clang",))
bench("bubble_astil_reg", "bench/bubble.af", ("./astil.exe", "bench/bubble.af"), setup=(BUILD,), tools=("clang",))
bench("bubble_astil_stack", "bench/bubble_s.af", ("./astil_s.exe", "bench/bubble_s.af"), setup=(BUILD,), tools=("clang",))
bench("bubble_gforth", "bench/bubble_g.fs", ("gforth", "bench/bubble_g.fs", "-e", "bye"), tools=("gforth",))
bench("bubble_luajit", "bench/bubble.lua", ("luajit", "bench/bubble.lua"), tools=("luajit",))
bench("bubble_js_bun", "bench/bubble.mjs", ("bun", "run", "bench/bubble.mjs"), tools=("bun",))
bench("bubble_cl_sbcl", "bench/bubble.lisp", ("sbcl", "--script", "bench/bubble.lisp"), tools=("sbcl",))
bench("bubble_pypy", "bench/bubble.py", ("pypy3", "bench/bubble.py"), tools=("pypy3",))
bench("bubble_python", "bench/bubble.py", ("python3", "bench/bubble.py"), tools=("python3",))

section("PRIME SIEVE")
bench("sieve_clang", "bench/sieve.c", ("bench/sieve.exe",), setup=c_exe("bench/sieve.exe"), tools=("clang",))
bench("sieve_astil_aot", "bench/sieve.af", ("bench/sieve_astil.exe",), setup=aot("bench/sieve.af", "bench/sieve_astil.exe"), tools=("clang",))
bench("sieve_astil_reg", "bench/sieve.af", ("./astil.exe", "bench/sieve.af"), setup=(BUILD,), tools=("clang",))
bench("sieve_astil_stack", "bench/sieve_s.af", ("./astil_s.exe", "bench/sieve_s.af"), setup=(BUILD,), tools=("clang",))
bench("sieve_gforth", "bench/sieve_g.fs", ("gforth", "bench/sieve_g.fs", "-e", "bye"), tools=("gforth",))
bench("sieve_luajit", "bench/sieve.lua", ("luajit", "bench/sieve.lua"), tools=("luajit",))
bench("sieve_js_bun", "bench/sieve.mjs", ("bun", "run", "bench/sieve.mjs"), tools=("bun",))
bench("sieve_cl_sbcl", "bench/sieve.lisp", ("sbcl", "--script", "bench/sieve.lisp"), tools=("sbcl",))
bench("sieve_pypy", "bench/sieve.py", ("pypy3", "bench/sieve.py"), tools=("pypy3",))
bench("sieve_python", "bench/sieve.py", ("python3", "bench/sieve.py"), tools=("python3",))

section("REVERSE STRING")
bench("reverse_string_clang", "bench/reverse_string.c", ("bench/reverse_string.exe",), setup=c_exe("bench/reverse_string.exe"), tools=("clang",))
bench("reverse_string_astil_aot", "bench/reverse_string.af", ("bench/reverse_string_astil.exe",), setup=aot("bench/reverse_string.af", "bench/reverse_string_astil.exe"), tools=("clang",))
bench("reverse_string_astil_reg", "bench/reverse_string.af", ("./astil.exe", "bench/reverse_string.af"), setup=(BUILD,), tools=("clang",))
bench("reverse_string_astil_stack", "bench/reverse_string_s.af", ("./astil_s.exe", "bench/reverse_string_s.af"), setup=(BUILD,), tools=("clang",))
bench("reverse_string_gforth", "bench/reverse_string_g.fs", ("gforth", "bench/reverse_string_g.fs", "-e", "bye"), tools=("gforth",))
bench("reverse_string_luajit", "bench/reverse_string.lua", ("luajit", "bench/reverse_string.lua"), tools=("luajit",))
bench("reverse_string_js_bun", "bench/reverse_string.mjs", ("bun", "run", "bench/reverse_string.mjs"), tools=("bun",))
bench("reverse_string_cl_sbcl", "bench/reverse_string.lisp", ("sbcl", "--script", "bench/reverse_string.lisp"), tools=("sbcl",))
bench("reverse_string_pypy", "bench/reverse_string.py", ("pypy3", "bench/reverse_string.py"), tools=("pypy3",))
bench("reverse_string_python", "bench/reverse_string.py", ("python3", "bench/reverse_string.py"), tools=("python3",))

section("FIB_LOOP")
bench("fib_loop_clang", "bench/fib_loop.c", ("bench/fib_loop.exe",), setup=c_exe("bench/fib_loop.exe"), tools=("clang",))
bench("fib_loop_astil_aot", "bench/fib_loop.af", ("bench/fib_loop_astil.exe",), setup=aot("bench/fib_loop.af", "bench/fib_loop_astil.exe"), tools=("clang",))
bench("fib_loop_astil_reg", "bench/fib_loop.af", ("./astil.exe", "bench/fib_loop.af"), setup=(BUILD,), tools=("clang",))
bench("fib_loop_astil_asm_aot", "bench/fib_asm.af", ("bench/fib_loop_astil_asm.exe",), setup=aot("bench/fib_asm.af", "bench/fib_loop_astil_asm.exe"), tools=("clang",))
bench("fib_loop_astil_stack", "bench/fib_loop_s.af", ("./astil_s.exe", "bench/fib_loop_s.af"), setup=(BUILD,), tools=("clang",))
bench("fib_loop_gforth", "bench/fib_loop_g.fs", ("gforth", "bench/fib_loop_g.fs", "-e", "bye"), tools=("gforth",))
bench("fib_loop_luajit", "bench/fib_loop.lua", ("luajit", "bench/fib_loop.lua"), tools=("luajit",))
bench("fib_loop_js_bun", "bench/fib_loop.mjs", ("bun", "run", "bench/fib_loop.mjs"), tools=("bun",))
bench("fib_loop_cl_sbcl", "bench/fib_loop.lisp", ("sbcl", "--script", "bench/fib_loop.lisp"), tools=("sbcl",))
bench("fib_loop_pypy", "bench/fib_loop.py", ("pypy3", "bench/fib_loop.py"), tools=("pypy3",))
bench("fib_loop_python", "bench/fib_loop.py", ("python3", "bench/fib_loop.py"), tools=("python3",))

section("FIB_LOOP_BIG")
bench("fib_loop_big_clang", "bench/fib_loop_big.c", ("bench/fib_loop_big.exe",), setup=c_exe("bench/fib_loop_big.exe"), tools=("clang",))
bench("fib_loop_big_astil_asm_aot", "bench/fib_loop_big_asm.af", ("bench/fib_loop_big_astil_asm.exe",), setup=aot("bench/fib_loop_big_asm.af", "bench/fib_loop_big_astil_asm.exe"), tools=("clang",))
bench("fib_loop_big_astil_asm_reg", "bench/fib_loop_big_asm.af", ("./astil.exe", "bench/fib_loop_big_asm.af"), setup=(BUILD,), tools=("clang",))
bench("fib_loop_big_astil_aot", "bench/fib_loop_big.af", ("bench/fib_loop_big_astil.exe",), setup=aot("bench/fib_loop_big.af", "bench/fib_loop_big_astil.exe"), tools=("clang",))
bench("fib_loop_big_astil_reg", "bench/fib_loop_big.af", ("./astil.exe", "bench/fib_loop_big.af"), setup=(BUILD,), tools=("clang",))
bench("fib_loop_big_js_bun", "bench/fib_loop_big.mjs", ("bun", "run", "bench/fib_loop_big.mjs"), tools=("bun",))
bench("fib_loop_big_cl_sbcl", "bench/fib_loop_big.lisp", ("sbcl", "--script", "bench/fib_loop_big.lisp"), tools=("sbcl",))
bench("fib_loop_big_pypy", "bench/fib_loop_big.py", ("pypy3", "bench/fib_loop_big.py"), tools=("pypy3",))
bench("fib_loop_big_python", "bench/fib_loop_big.py", ("python3", "bench/fib_loop_big.py"), tools=("python3",))

section("FIB_RECURSIVE: fib(39)")
bench("fib_rec_clang", "bench/fib_rec.c", ("bench/fib_rec.exe",), setup=c_exe("bench/fib_rec.exe"), tools=("clang",))
bench("fib_rec_astil_aot", "bench/fib_rec.af", ("bench/fib_rec_astil.exe",), setup=aot("bench/fib_rec.af", "bench/fib_rec_astil.exe"), tools=("clang",))
bench("fib_rec_astil_reg", "bench/fib_rec.af", ("./astil.exe", "bench/fib_rec.af"), setup=(BUILD,), tools=("clang",))
bench("fib_rec_astil_stack", "bench/fib_rec_s.af", ("./astil_s.exe", "bench/fib_rec_s.af"), setup=(BUILD,), tools=("clang",))
bench("fib_rec_gforth", "bench/fib_rec_g.fs", ("gforth", "bench/fib_rec_g.fs", "-e", "bye"), tools=("gforth",))
bench("fib_rec_luajit", "bench/fib_rec.lua", ("luajit", "bench/fib_rec.lua"), tools=("luajit",))
bench("fib_rec_js_bun", "bench/fib_rec.mjs", ("bun", "run", "bench/fib_rec.mjs"), tools=("bun",))
bench("fib_rec_cl_sbcl", "bench/fib_rec.lisp", ("sbcl", "--script", "bench/fib_rec.lisp"), tools=("sbcl",))
bench("fib_rec_pypy", "bench/fib_rec.py", ("pypy3", "bench/fib_rec.py"), tools=("pypy3",))
bench("fib_rec_python", "bench/fib_rec.py", ("python3", "bench/fib_rec.py"), tools=("python3",))

section("CONST FOLD")
bench("const_fold_folded_astil_aot", "bench/const_fold_folded.af", ("bench/const_fold_folded_astil.exe",), setup=aot("bench/const_fold_folded.af", "bench/const_fold_folded_astil.exe"), tools=("clang",))
bench("const_fold_runtime_astil_aot", "bench/const_fold_runtime.af", ("bench/const_fold_runtime_astil.exe",), setup=aot("bench/const_fold_runtime.af", "bench/const_fold_runtime_astil.exe"), tools=("clang",))
bench("const_fold_folded_astil_reg", "bench/const_fold_folded.af", ("./astil.exe", "bench/const_fold_folded.af"), setup=(BUILD,), tools=("clang",))
bench("const_fold_runtime_astil_reg", "bench/const_fold_runtime.af", ("./astil.exe", "bench/const_fold_runtime.af"), setup=(BUILD,), tools=("clang",))

section("FNV-1A 64")
bench("fnv1a64_clang", "bench/fnv1a64.c", ("bench/fnv1a64.exe",), setup=c_exe("bench/fnv1a64.exe"), tools=("clang",))
bench("fnv1a64_astil_aot", "bench/fnv1a64.af", ("bench/fnv1a64_astil.exe",), setup=aot("bench/fnv1a64.af", "bench/fnv1a64_astil.exe"), tools=("clang",))
bench("fnv1a64_astil_reg", "bench/fnv1a64.af", ("./astil.exe", "bench/fnv1a64.af"), setup=(BUILD,), tools=("clang",))
bench("fnv1a64_astil_stack", "bench/fnv1a64_s.af", ("./astil_s.exe", "bench/fnv1a64_s.af"), setup=(BUILD,), tools=("clang",))
bench("fnv1a64_gforth", "bench/fnv1a64_g.fs", ("gforth", "bench/fnv1a64_g.fs", "-e", "bye"), tools=("gforth",))
bench("fnv1a64_luajit", "bench/fnv1a64.lua", ("luajit", "bench/fnv1a64.lua"), tools=("luajit",))
bench("fnv1a64_js_bun", "bench/fnv1a64.mjs", ("bun", "run", "bench/fnv1a64.mjs"), tools=("bun",))
bench("fnv1a64_cl_sbcl", "bench/fnv1a64.lisp", ("sbcl", "--script", "bench/fnv1a64.lisp"), tools=("sbcl",))
bench("fnv1a64_pypy", "bench/fnv1a64.py", ("pypy3", "bench/fnv1a64.py"), tools=("pypy3",))
bench("fnv1a64_python", "bench/fnv1a64.py", ("python3", "bench/fnv1a64.py"), tools=("python3",))

section("SCAN DELIMS")
bench("scan_delims_c_simd", "bench/scan_delims_simd.c", ("bench/scan_delims_simd.exe",), setup=c_exe("bench/scan_delims_simd.exe"), tools=("clang",))
bench("scan_delims_c_naive", "bench/scan_delims_naive.c", ("bench/scan_delims_naive.exe",), setup=c_exe("bench/scan_delims_naive.exe"), tools=("clang",))
bench("scan_delims_astil_simd_aot", "bench/scan_delims_simd.af", ("bench/scan_delims_simd_astil.exe",), setup=aot("bench/scan_delims_simd.af", "bench/scan_delims_simd_astil.exe"), tools=("clang",))
bench("scan_delims_astil_simd_reg", "bench/scan_delims_simd.af", ("./astil.exe", "bench/scan_delims_simd.af"), setup=(BUILD,), tools=("clang",))
bench("scan_delims_astil_naive_reg", "bench/scan_delims_naive.af", ("./astil.exe", "bench/scan_delims_naive.af"), setup=(BUILD,), tools=("clang",))
bench("scan_delims_luajit", "bench/scan_delims.lua", ("luajit", "bench/scan_delims.lua"), tools=("luajit",))
bench("scan_delims_js_bun", "bench/scan_delims.mjs", ("bun", "run", "bench/scan_delims.mjs"), tools=("bun",))
bench("scan_delims_cl_sbcl", "bench/scan_delims.lisp", ("sbcl", "--script", "bench/scan_delims.lisp"), tools=("sbcl",))
bench("scan_delims_pypy", "bench/scan_delims.py", ("pypy3", "bench/scan_delims.py"), tools=("pypy3",))
bench("scan_delims_python", "bench/scan_delims_cpython.py", ("python3", "bench/scan_delims_cpython.py"), tools=("python3",))

section("BINARY TREE")
bench("bin_tree_clang", "bench/bin_tree.c", ("bench/bin_tree.exe",), setup=c_exe("bench/bin_tree.exe"), tools=("clang",))
bench("bin_tree_astil_aot", "bench/bin_tree.af", ("bench/bin_tree_astil.exe",), setup=aot("bench/bin_tree.af", "bench/bin_tree_astil.exe"), tools=("clang",))
bench("bin_tree_astil_reg", "bench/bin_tree.af", ("./astil.exe", "bench/bin_tree.af"), setup=(BUILD,), tools=("clang",))
bench("bin_tree_astil_stack", "bench/bin_tree_s.af", ("./astil_s.exe", "bench/bin_tree_s.af"), setup=(BUILD,), tools=("clang",))
bench("bin_tree_gforth", "bench/bin_tree_g.fs", ("gforth", "bench/bin_tree_g.fs", "-e", "bye"), tools=("gforth",))
bench("bin_tree_zig", "bench/bin_tree.zig", ("bench/bin_tree_zig.exe",), setup=zig_exe("bench/bin_tree.zig", "bench/bin_tree_zig.exe"), tools=("zig",))
bench("bin_tree_go", "bench/bin_tree.go", ("bench/bin_tree_go.exe",), setup=go_exe("bench/bin_tree.go", "bench/bin_tree_go.exe"), tools=("go",))
bench("bin_tree_luajit", "bench/bin_tree.lua", ("luajit", "bench/bin_tree.lua"), tools=("luajit",))
bench("bin_tree_js_bun", "bench/bin_tree.mjs", ("bun", "run", "bench/bin_tree.mjs"), tools=("bun",))
bench("bin_tree_js_bun_lucky", "bench/bin_tree_lucky.mjs", ("bun", "run", "bench/bin_tree_lucky.mjs"), tools=("bun",))
bench("bin_tree_java", "bench/bin_tree.java", ("java", "-cp", "bench", "bin_tree"), setup=java_class("bench/bin_tree.java"), tools=("java",))
bench("bin_tree_cl_sbcl", "bench/bin_tree.lisp", ("sbcl", "--script", "bench/bin_tree.lisp"), tools=("sbcl",))
bench("bin_tree_pypy", "bench/bin_tree.py", ("pypy3", "bench/bin_tree.py"), tools=("pypy3",))
bench("bin_tree_python", "bench/bin_tree.py", ("python3", "bench/bin_tree.py"), tools=("python3",))

section("BINARY TREE BULK")
bench("bin_tree_clang_bulk", "bench/bin_tree_bulk.c", ("bench/bin_tree_bulk.exe",), setup=c_exe("bench/bin_tree_bulk.exe"), tools=("clang",))
bench("bin_tree_astil_aot_bulk", "bench/bin_tree_bulk.af", ("bench/bin_tree_bulk_astil.exe",), setup=aot("bench/bin_tree_bulk.af", "bench/bin_tree_bulk_astil.exe"), tools=("clang",))
bench("bin_tree_astil_reg_bulk", "bench/bin_tree_bulk.af", ("./astil.exe", "bench/bin_tree_bulk.af"), setup=(BUILD,), tools=("clang",))
bench("bin_tree_astil_stack_bulk", "bench/bin_tree_bulk_s.af", ("./astil_s.exe", "bench/bin_tree_bulk_s.af"), setup=(BUILD,), tools=("clang",))
bench("bin_tree_gforth_bulk", "bench/bin_tree_bulk_g.fs", ("gforth", "bench/bin_tree_bulk_g.fs", "-e", "bye"), tools=("gforth",))
bench("bin_tree_go_bulk", "bench/bin_tree_bulk.go", ("bench/bin_tree_go_bulk.exe",), setup=go_exe("bench/bin_tree_bulk.go", "bench/bin_tree_go_bulk.exe"), tools=("go",))


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("filters", nargs="*", help="AND filters; spaces inside one arg are OR; globs supported")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="default: generated/bench.md")
    parser.add_argument("--smoke", action="store_true")
    return parser.parse_args(argv)


def matches_pattern(value: str, pattern: str) -> bool:
    if fnmatch.fnmatchcase(value, pattern):
        return True
    if not any(char in pattern for char in "*?["):
        return fnmatch.fnmatchcase(value, f"*{pattern}*")
    return False


def matches_any(value: str, patterns: list[str]) -> bool:
    return any(matches_pattern(value, pattern) for pattern in patterns)


def matches_filter(item: Bench, group: str) -> bool:
    patterns = group.split()
    base = Path(item.file).name
    return (
        matches_any(item.file, patterns)
        or matches_any(base, patterns)
        or matches_any(item.name, patterns)
    )


def selected_benches(filters: list[str]) -> list[Bench]:
    groups = filters or ["*"]
    return [item for item in BENCHES if all(matches_filter(item, group) for group in groups)]


def run(cmd: tuple[str, ...], *, capture: bool = False) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            cmd,
            cwd=ROOT,
            check=True,
            text=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.STDOUT if capture else None,
        )
    except subprocess.CalledProcessError as err:
        if err.stdout:
            print(err.stdout, file=sys.stderr, end="" if err.stdout.endswith("\n") else "\n")
        raise


def progress(msg: str) -> None:
    print(f"[bench] {msg}", file=sys.stderr)


def unique_setup(items: list[Bench]) -> list[tuple[str, ...]]:
    seen: set[tuple[str, ...]] = set()
    result: list[tuple[str, ...]] = []
    for item in items:
        for cmd in item.setup:
            if cmd in seen:
                continue
            seen.add(cmd)
            result.append(cmd)
    return result


def uses_astil(items: list[Bench]) -> bool:
    return any("astil" in item.name for item in items)


def selected_tools(items: list[Bench]) -> list[str]:
    seen: set[str] = set()
    result: list[str] = []
    for item in items:
        for tool in item.tools:
            if tool in seen:
                continue
            seen.add(tool)
            result.append(tool)
    return result


def write_versions(out, items: list[Bench]) -> None:
    out.write("## VERSIONS\n\n```\n")
    for tool in selected_tools(items):
        progress(f"version {tool}")
        out.write(run(TOOLS[tool], capture=True).stdout.strip())
        out.write("\n\n")
    out.write("```\n")
    out.flush()


def summarize(
    samples: list[Sample], field: str
) -> tuple[float, float | None]:
    values = [getattr(sample, field) for sample in samples]
    # One run has a mean but no sample standard deviation.
    deviation = statistics.stdev(values) if len(values) > 1 else None
    return statistics.mean(values), deviation


def format_seconds(seconds: float) -> str:
    if seconds < 0.001:
        return f"{seconds * 1_000_000:.1f} µs"
    if seconds < 1:
        return f"{seconds * 1_000:.1f} ms"
    return f"{seconds:.1f} s"


def duration_unit(seconds: float) -> tuple[str, float]:
    """Choose one readable unit from a table column's typical mean."""
    if seconds < 0.001:
        return "µs", 1_000_000
    if seconds < 1:
        return "ms", 1_000
    return "s", 1


def format_scaled_measurement(
    mean: float,
    deviation: float | None,
    unit: str,
    scale: float,
    *,
    include_unit: bool,
) -> str:
    suffix = f" {unit}" if include_unit else ""
    if deviation is None:
        return f"{mean * scale:.1f}{suffix} ± —"
    scaled_deviation = deviation * scale
    if not scaled_deviation:
        return f"{mean * scale:.1f}{suffix} ± 0.0{suffix}"
    wanted_places = max(
        1,
        1 - math.floor(math.log10(abs(scaled_deviation))),
    )
    decimal_places = min(wanted_places, 3)
    formatted_mean = f"{mean * scale:.{decimal_places}f}"
    formatted_deviation = (
        f"{scaled_deviation:.1e}"
        if wanted_places > 3 and round(scaled_deviation, 3) == 0
        else f"{scaled_deviation:.{decimal_places}f}"
    )
    return (
        f"{formatted_mean}{suffix} ± "
        f"{formatted_deviation}{suffix}"
    )


def format_measurement(
    mean: float, deviation: float | None, formatter
) -> str:
    formatted_mean = formatter(mean)
    unit = formatted_mean.split()[1]
    scale = {"µs": 1_000_000, "ms": 1_000, "s": 1, "MiB": 1 / 1024**2}[unit]
    return format_scaled_measurement(
        mean,
        deviation,
        unit,
        scale,
        include_unit=True,
    )


def format_table_measurement(
    mean: float,
    deviation: float | None,
    unit: str,
    scale: float,
) -> str:
    return format_scaled_measurement(
        mean,
        deviation,
        unit,
        scale,
        include_unit=False,
    )


def format_mib(bytes_: float) -> str:
    return f"{bytes_ / 1024**2:.1f} MiB"


def render_section(title: str, results: list[Result]) -> str:
    results = sorted(
        results,
        key=lambda result: summarize(result.samples, "wall_seconds")[0],
    )
    wall_means = [
        summarize(result.samples, "wall_seconds")[0]
        for result in results
    ]
    fastest = wall_means[0]
    wall_unit, wall_scale = duration_unit(statistics.median(wall_means))
    cpu_means = [
        summarize(result.samples, "cpu_seconds")[0]
        for result in results
    ]
    cpu_unit, cpu_scale = duration_unit(statistics.median(cpu_means))
    lines = [
        f"\n## {title}\n",
        f"| Command | Wall [{wall_unit}] | CPU [{cpu_unit}] | "
        "Peak mem [MiB] | Relative |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    warnings = []
    for result in results:
        wall_mean, wall_deviation = summarize(result.samples, "wall_seconds")
        cpu_mean, cpu_deviation = summarize(result.samples, "cpu_seconds")
        rss_mean, rss_deviation = summarize(result.samples, "peak_rss_bytes")
        wall = format_table_measurement(
            wall_mean, wall_deviation, wall_unit, wall_scale
        )
        cpu = format_table_measurement(
            cpu_mean, cpu_deviation, cpu_unit, cpu_scale
        )
        mem = format_table_measurement(
            rss_mean, rss_deviation, "MiB", 1 / 1024**2
        )
        lines.append(
            f"| `{result.item.name}` | "
            f"{wall} | {cpu} | {mem} | "
            f"{wall_mean / fastest:.2f} |"
        )
        instability = wall_instability(result.samples)
        if instability is not None:
            warnings.append(
                f"`{result.item.name}` (middle 50% spans {instability:.1%})"
            )
    if warnings:
        lines.extend(("", "> Warning: unstable wall time: " + "; ".join(warnings) + "."))
    return "\n".join(lines) + "\n"


TTY_CLEAR = "\r\x1b[2K"


def format_runs(count: int) -> str:
    return f"{count} run" + ("" if count == 1 else "s")


def format_estimate(
    name: str, completed: int, samples: list[Sample]
) -> str:
    wall_mean = statistics.mean(sample.wall_seconds for sample in samples)
    cpu_mean = statistics.mean(sample.cpu_seconds for sample in samples)
    rss_mean = statistics.mean(sample.peak_rss_bytes for sample in samples)
    return (
        f"[bench] [{name}] {format_runs(completed)}; "
        f"wall ≈{format_seconds(wall_mean)}; "
        f"CPU ≈{format_seconds(cpu_mean)}; "
        f"mem ≈{format_mib(rss_mean)}"
    )


def format_done(result: Result) -> str:
    wall_mean, wall_deviation = summarize(result.samples, "wall_seconds")
    cpu_mean, cpu_deviation = summarize(result.samples, "cpu_seconds")
    rss_mean, rss_deviation = summarize(result.samples, "peak_rss_bytes")
    return (
        f"[bench] [{result.item.name}] done: "
        f"wall {format_measurement(wall_mean, wall_deviation, format_seconds)}, "
        f"CPU {format_measurement(cpu_mean, cpu_deviation, format_seconds)}, "
        f"mem {format_measurement(rss_mean, rss_deviation, format_mib)}; "
        f"{format_runs(len(result.samples))}"
    )


def validate(items: list[Bench]) -> None:
    for item in items:
        progress(f"[{item.name}] validation")
        try:
            measure(item, MAX_MEASURE_SECONDS)
        except MeasurementDeadline:
            raise RuntimeError(
                f"benchmark {item.name!r} validation exceeded "
                f"{MAX_MEASURE_SECONDS:g} seconds: {item.cmd!r}"
            ) from None


def measure_bench(item: Bench) -> Result:
    tty = sys.stderr.isatty()
    print(
        f"[bench] [{item.name}] starting; "
        f"{MAX_MEASURE_SECONDS:g} second limit",
        file=sys.stderr,
    )
    deadline = time.perf_counter() + MAX_MEASURE_SECONDS
    next_progress = time.perf_counter() + PROGRESS_SECONDS
    samples = []
    while True:
        remaining = deadline - time.perf_counter()
        if remaining <= 0:
            break
        try:
            samples.append(measure(item, remaining))
        except MeasurementDeadline:
            # Deadline interrupted an incomplete run; it is not a sample.
            break
        now = time.perf_counter()
        if tty and now >= next_progress:
            print(
                TTY_CLEAR + format_estimate(item.name, len(samples), samples),
                end="",
                file=sys.stderr,
                flush=True,
            )
            next_progress = now + PROGRESS_SECONDS
        if measurement_stable(samples):
            break
    if not samples:
        raise RuntimeError(
            f"benchmark {item.name!r} completed no runs within "
            f"{MAX_MEASURE_SECONDS:g} seconds: {item.cmd!r}"
        )
    if tty:
        print(TTY_CLEAR, end="", file=sys.stderr)
    if not measurement_stable(samples):
        print(
            f"[bench] [{item.name}] warning: precision target not reached "
            f"before {MAX_MEASURE_SECONDS:g} second limit",
            file=sys.stderr,
        )
    result = Result(item, samples)
    print(format_done(result), file=sys.stderr, flush=True)
    return result


def run_section(
    out,
    title: str,
    items: list[Bench],
) -> None:
    if not items:
        return

    results = [measure_bench(item) for item in items]
    out.write(render_section(title, results))
    out.flush()


def main_for(argv: list[str]) -> int:
    args = parse_args(argv)
    items = selected_benches(args.filters)

    if not items:
        print("no benchmarks match filters:", *args.filters, file=sys.stderr)
        return 2

    os.chdir(ROOT)

    if uses_astil(items):
        progress("setup " + " ".join(CLEAN))
        run(CLEAN)

    output = Path(args.output)
    if not output.is_absolute():
        output = ROOT / output

    for cmd in unique_setup(items):
        progress("setup " + " ".join(cmd))
        run(cmd)

    validate(items)
    if args.smoke:
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as out:
        write_versions(out, items)
        for title in SECTIONS:
            run_section(
                out,
                title,
                [item for item in items if item.section == title],
            )

    progress(f"wrote {output.relative_to(ROOT) if output.is_relative_to(ROOT) else output}")
    return 0


def main() -> int:
    return main_for(sys.argv[1:])


if __name__ == "__main__":
    raise SystemExit(main())
