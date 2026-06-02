#! /usr/bin/env python3

# BOT-GENERATED

import argparse
import fnmatch
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
GEN = ROOT / "generated"
DEFAULT_OUTPUT = GEN / "bench.md"
SECTION_OUTPUT = GEN / f".bench-section-{os.getpid()}.md"

HYPERFINE_OPTS = [
    "--warmup=8",
    "--shell=none",
]

BUILD = ("make", "PROD=true", "build")

TOOLS = {
    "clang": ("clang", "--version"),
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
    cmd: str
    setup: tuple[tuple[str, ...], ...] = ()
    tools: tuple[str, ...] = ()


BENCHES: list[Bench] = []
SECTIONS: list[str] = []


def section(title: str) -> None:
    SECTIONS.append(title)


def bench(
    name: str,
    file: str,
    cmd: str,
    *,
    setup: tuple[tuple[str, ...], ...] = (),
    tools: tuple[str, ...] = (),
) -> None:
    BENCHES.append(Bench(SECTIONS[-1], name, file, cmd, setup, tools))


def c_exe(src: str, exe: str) -> tuple[tuple[str, ...], ...]:
    return (("make", "PROD=true", exe),)


def aot(src: str, exe: str) -> tuple[tuple[str, ...], ...]:
    return (BUILD, ("./astil.exe", src, f"--build={exe}"))


section("NONE")
bench("none_astil_reg", "astil.exe", "./astil.exe", setup=(BUILD,), tools=("clang",))
bench("none_astil_stack", "astil_s.exe", "./astil_s.exe", setup=(BUILD,), tools=("clang",))

section("BASELINE")
bench("baseline_astil_reg", "forth/lang.af", "./astil.exe forth/lang.af", setup=(BUILD,), tools=("clang",))
bench("baseline_astil_stack", "forth/lang_s.af", "./astil_s.exe forth/lang_s.af", setup=(BUILD,), tools=("clang",))
bench("baseline_gforth", "gforth", "gforth -e bye", tools=("gforth",))
bench("baseline_luajit", "luajit", 'luajit -e ""', tools=("luajit",))
bench("baseline_js_bun", "bun", 'bun -e ""', tools=("bun",))
bench("baseline_cl_sbcl", "sbcl", "sbcl --noinform --non-interactive", tools=("sbcl",))
bench("baseline_pypy", "pypy3", 'pypy3 -c ""', tools=("pypy3",))
bench("baseline_python", "python3", 'python3 -c ""', tools=("python3",))

section("BUBBLE")
bench("bubble_clang", "bench/bubble.c", "bench/bubble.exe", setup=c_exe("bench/bubble.c", "bench/bubble.exe"), tools=("clang",))
bench("bubble_astil_aot", "bench/bubble.af", "bench/bubble_astil.exe", setup=aot("bench/bubble.af", "bench/bubble_astil.exe"), tools=("clang",))
bench("bubble_astil_reg", "bench/bubble.af", "./astil.exe bench/bubble.af", setup=(BUILD,), tools=("clang",))
bench("bubble_astil_stack", "bench/bubble_s.af", "./astil_s.exe bench/bubble_s.af", setup=(BUILD,), tools=("clang",))
bench("bubble_gforth", "bench/bubble_g.fs", "gforth bench/bubble_g.fs -e bye", tools=("gforth",))
bench("bubble_luajit", "bench/bubble.lua", "luajit bench/bubble.lua", tools=("luajit",))
bench("bubble_js_bun", "bench/bubble.mjs", "bun run bench/bubble.mjs", tools=("bun",))
bench("bubble_cl_sbcl", "bench/bubble.lisp", "sbcl --script bench/bubble.lisp", tools=("sbcl",))
bench("bubble_pypy", "bench/bubble.py", "pypy3 bench/bubble.py", tools=("pypy3",))
bench("bubble_python", "bench/bubble.py", "python3 bench/bubble.py", tools=("python3",))

section("PRIME SIEVE")
bench("sieve_clang", "bench/sieve.c", "bench/sieve.exe", setup=c_exe("bench/sieve.c", "bench/sieve.exe"), tools=("clang",))
bench("sieve_astil_aot", "bench/sieve.af", "bench/sieve_astil.exe", setup=aot("bench/sieve.af", "bench/sieve_astil.exe"), tools=("clang",))
bench("sieve_astil_reg", "bench/sieve.af", "./astil.exe bench/sieve.af", setup=(BUILD,), tools=("clang",))
bench("sieve_astil_stack", "bench/sieve_s.af", "./astil_s.exe bench/sieve_s.af", setup=(BUILD,), tools=("clang",))
bench("sieve_gforth", "bench/sieve_g.fs", "gforth bench/sieve_g.fs -e bye", tools=("gforth",))
bench("sieve_luajit", "bench/sieve.lua", "luajit bench/sieve.lua", tools=("luajit",))
bench("sieve_js_bun", "bench/sieve.mjs", "bun run bench/sieve.mjs", tools=("bun",))
bench("sieve_cl_sbcl", "bench/sieve.lisp", "sbcl --script bench/sieve.lisp", tools=("sbcl",))
bench("sieve_pypy", "bench/sieve.py", "pypy3 bench/sieve.py", tools=("pypy3",))
bench("sieve_python", "bench/sieve.py", "python3 bench/sieve.py", tools=("python3",))

section("FIB_LOOP")
bench("fib_loop_clang", "bench/fib_loop.c", "bench/fib_loop.exe", setup=c_exe("bench/fib_loop.c", "bench/fib_loop.exe"), tools=("clang",))
bench("fib_loop_astil_aot", "bench/fib_loop.af", "bench/fib_loop_astil.exe", setup=aot("bench/fib_loop.af", "bench/fib_loop_astil.exe"), tools=("clang",))
bench("fib_loop_astil_reg", "bench/fib_loop.af", "./astil.exe bench/fib_loop.af", setup=(BUILD,), tools=("clang",))
bench("fib_loop_astil_asm", "bench/fib_asm.af", "./astil.exe bench/fib_asm.af", setup=(BUILD,), tools=("clang",))
bench("fib_loop_astil_stack", "bench/fib_loop_s.af", "./astil_s.exe bench/fib_loop_s.af", setup=(BUILD,), tools=("clang",))
bench("fib_loop_gforth", "bench/fib_loop_g.fs", "gforth bench/fib_loop_g.fs -e bye", tools=("gforth",))
bench("fib_loop_luajit", "bench/fib_loop.lua", "luajit bench/fib_loop.lua", tools=("luajit",))
bench("fib_loop_js_bun", "bench/fib_loop.mjs", "bun run bench/fib_loop.mjs", tools=("bun",))
bench("fib_loop_cl_sbcl", "bench/fib_loop.lisp", "sbcl --script bench/fib_loop.lisp", tools=("sbcl",))
bench("fib_loop_pypy", "bench/fib_loop.py", "pypy3 bench/fib_loop.py", tools=("pypy3",))
bench("fib_loop_python", "bench/fib_loop.py", "python3 bench/fib_loop.py", tools=("python3",))

section("FIB_LOOP_BIG")
bench("fib_loop_big_js_bun", "bench/fib_loop_big.mjs", "bun run bench/fib_loop_big.mjs", tools=("bun",))
bench("fib_loop_big_cl_sbcl", "bench/fib_loop_big.lisp", "sbcl --script bench/fib_loop_big.lisp", tools=("sbcl",))
bench("fib_loop_big_pypy", "bench/fib_loop_big.py", "pypy3 bench/fib_loop_big.py", tools=("pypy3",))
bench("fib_loop_big_python", "bench/fib_loop_big.py", "python3 bench/fib_loop_big.py", tools=("python3",))

section("FIB_RECURSIVE: fib(36)")
bench("fib_rec_clang", "bench/fib_rec.c", "bench/fib_rec.exe", setup=c_exe("bench/fib_rec.c", "bench/fib_rec.exe"), tools=("clang",))
bench("fib_rec_astil_aot", "bench/fib_rec.af", "bench/fib_rec_astil.exe", setup=aot("bench/fib_rec.af", "bench/fib_rec_astil.exe"), tools=("clang",))
bench("fib_rec_astil_reg", "bench/fib_rec.af", "./astil.exe bench/fib_rec.af", setup=(BUILD,), tools=("clang",))
bench("fib_rec_astil_stack", "bench/fib_rec_s.af", "./astil_s.exe bench/fib_rec_s.af", setup=(BUILD,), tools=("clang",))
bench("fib_rec_gforth", "bench/fib_rec_g.fs", "gforth bench/fib_rec_g.fs -e bye", tools=("gforth",))
bench("fib_rec_luajit", "bench/fib_rec.lua", "luajit bench/fib_rec.lua", tools=("luajit",))
bench("fib_rec_js_bun", "bench/fib_rec.mjs", "bun run bench/fib_rec.mjs", tools=("bun",))
bench("fib_rec_cl_sbcl", "bench/fib_rec.lisp", "sbcl --script bench/fib_rec.lisp", tools=("sbcl",))
bench("fib_rec_pypy", "bench/fib_rec.py", "pypy3 bench/fib_rec.py", tools=("pypy3",))
bench("fib_rec_python", "bench/fib_rec.py", "python3 bench/fib_rec.py", tools=("python3",))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("filters", nargs="*", help="AND filters; spaces inside one arg are OR; globs supported")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="default: generated/bench.md")
    parser.add_argument("--no-progress", action="store_true")
    return parser.parse_args()


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


def progress(enabled: bool, msg: str) -> None:
    if enabled:
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


def write_versions(out, items: list[Bench], show_progress: bool) -> None:
    out.write("## VERSIONS\n\n```\n")
    for tool in selected_tools(items):
        progress(show_progress, f"version {tool}")
        out.write(run(TOOLS[tool], capture=True).stdout.strip())
        out.write("\n\n")
    out.write("```\n")
    out.flush()


def run_section(out, title: str, items: list[Bench], show_progress: bool) -> None:
    if not items:
        return

    progress(show_progress, f"bench {title}: {', '.join(item.name for item in items)}")
    args = ["hyperfine", *HYPERFINE_OPTS, f"--export-markdown={SECTION_OUTPUT}"]
    if not show_progress:
        args.append("--style=none")
    for item in items:
        args.extend(["--command-name", item.name, item.cmd])

    out.write(f"\n## {title}\n\n")
    out.flush()
    SECTION_OUTPUT.unlink(missing_ok=True)
    try:
        run(tuple(args), capture=not show_progress)
        rendered = SECTION_OUTPUT.read_text(encoding="utf-8")
    finally:
        SECTION_OUTPUT.unlink(missing_ok=True)
    out.write(rendered)
    if not rendered.endswith("\n"):
        out.write("\n")
    out.flush()


def main() -> int:
    args = parse_args()
    show_progress = not args.no_progress
    items = selected_benches(args.filters)

    if not items:
        print("no benchmarks match filters:", *args.filters, file=sys.stderr)
        return 2

    os.chdir(ROOT)
    GEN.mkdir(exist_ok=True)
    output = Path(args.output)
    if not output.is_absolute():
        output = ROOT / output
    output.parent.mkdir(parents=True, exist_ok=True)

    for cmd in unique_setup(items):
        progress(show_progress, "setup " + " ".join(cmd))
        run(cmd)

    with output.open("w", encoding="utf-8") as out:
        write_versions(out, items, show_progress)
        for title in SECTIONS:
            run_section(out, title, [item for item in items if item.section == title], show_progress)

    progress(show_progress, f"wrote {output.relative_to(ROOT) if output.is_relative_to(ROOT) else output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
