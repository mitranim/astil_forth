#! /usr/bin/env sh

# This file is mostly bot-generated.

# Non-zero exit = exception.
set -e

# Keep filter patterns like `*` and `*.mjs` literal until `case` matches them.
set -f

# Ensure we're in repo root.
cd "$(realpath "$(dirname -- "$0")")/.."

FILTERS=$(printf '%s\n' "${@:-*}")

# OPTS+=' --warmup=8 --shell=none --show-output'
OPTS+=' --warmup=8 --shell=none'
MATCHED=false
GEN=generated

ALL_FILES='
  astil.exe astil_s.exe gforth sbcl
  forth/lang.af forth/lang_s.af
  bench/bubble.c bench/bubble.af bench/bubble_s.af bench/bubble_g.fs bench/bubble.lua bench/bubble.mjs bench/bubble.lisp bench/bubble.py
  bench/sieve.c bench/sieve.af bench/sieve_s.af bench/sieve_g.fs bench/sieve.lua bench/sieve.mjs bench/sieve.lisp bench/sieve.py
  bench/fib_loop.c bench/fib_loop.af bench/fib_asm.af bench/fib_loop_s.af bench/fib_loop_g.fs bench/fib_loop.lua bench/fib_loop.mjs bench/fib_loop.lisp bench/fib_loop.py
  bench/fib_loop_big.mjs bench/fib_loop_big.lisp bench/fib_loop_big.py
  bench/fib_rec.c bench/fib_rec.af bench/fib_rec_s.af bench/fib_rec_g.fs bench/fib_rec.lua bench/fib_rec.mjs bench/fib_rec.lisp bench/fib_rec.py
'

matches_filter() {
  local path=$1
  local filter=$2
  local base=${path##*/}
  local pattern

  for pattern in $filter
  do
    case "$path" in
      $pattern) return 0 ;;
    esac

    case "$base" in
      $pattern) return 0 ;;
    esac
  done

  return 1
}

matches() {
  local path=$1
  local filter

  while IFS= read -r filter
  do
    if ! matches_filter "$path" "$filter"
    then return 1
    fi
  done <<EOF
$FILTERS
EOF

  return 0
}

has_match() {
  local file

  for file in $ALL_FILES
  do
    if matches "$file"
    then return 0
    fi
  done

  return 1
}

run_section() {
  local title=$1
  local files="${@:2}"
  local file

  set --

  for file in $files
  do
    if ! matches "$file"
    then continue
    fi

    case "$file" in
      astil.exe)
        set -- "$@" --command-name none_astil_reg './astil.exe'
        ;;
      astil_s.exe)
        set -- "$@" --command-name none_astil_stack './astil_s.exe'
        ;;
      gforth)
        set -- "$@" --command-name baseline_gforth 'gforth -e bye'
        ;;
      sbcl)
        set -- "$@" --command-name baseline_cl_sbcl 'sbcl --noinform --non-interactive'
        ;;
      forth/lang.af)
        set -- "$@" --command-name baseline_astil_reg './astil.exe forth/lang.af'
        ;;
      forth/lang_s.af)
        set -- "$@" --command-name baseline_astil_stack './astil_s.exe forth/lang_s.af'
        ;;
      bench/bubble.c)
        set -- "$@" --command-name bubble_clang 'bench/bubble.exe'
        ;;
      bench/bubble.af)
        set -- "$@" --command-name bubble_astil_aot 'bench/bubble_astil.exe'
        set -- "$@" --command-name bubble_astil_reg './astil.exe bench/bubble.af'
        ;;
      bench/bubble_s.af)
        set -- "$@" --command-name bubble_astil_stack './astil_s.exe bench/bubble_s.af'
        ;;
      bench/bubble_g.fs)
        set -- "$@" --command-name bubble_gforth 'gforth bench/bubble_g.fs -e bye'
        ;;
      bench/bubble.lua)
        set -- "$@" --command-name bubble_luajit 'luajit bench/bubble.lua'
        ;;
      bench/bubble.mjs)
        set -- "$@" --command-name bubble_js_bun 'bun run bench/bubble.mjs'
        ;;
      bench/bubble.lisp)
        set -- "$@" --command-name bubble_cl_sbcl 'sbcl --script bench/bubble.lisp'
        ;;
      bench/bubble.py)
        set -- "$@" --command-name bubble_pypy 'pypy3 bench/bubble.py'
        set -- "$@" --command-name bubble_python 'python3 bench/bubble.py'
        ;;
      bench/sieve.c)
        set -- "$@" --command-name sieve_clang 'bench/sieve.exe'
        ;;
      bench/sieve.af)
        set -- "$@" --command-name sieve_astil_aot 'bench/sieve_astil.exe'
        set -- "$@" --command-name sieve_astil_reg './astil.exe bench/sieve.af'
        ;;
      bench/sieve_s.af)
        set -- "$@" --command-name sieve_astil_stack './astil_s.exe bench/sieve_s.af'
        ;;
      bench/sieve_g.fs)
        set -- "$@" --command-name sieve_gforth 'gforth bench/sieve_g.fs -e bye'
        ;;
      bench/sieve.lua)
        set -- "$@" --command-name sieve_luajit 'luajit bench/sieve.lua'
        ;;
      bench/sieve.mjs)
        set -- "$@" --command-name sieve_js_bun 'bun run bench/sieve.mjs'
        ;;
      bench/sieve.lisp)
        set -- "$@" --command-name sieve_cl_sbcl 'sbcl --script bench/sieve.lisp'
        ;;
      bench/sieve.py)
        set -- "$@" --command-name sieve_pypy 'pypy3 bench/sieve.py'
        set -- "$@" --command-name sieve_python 'python3 bench/sieve.py'
        ;;
      bench/fib_loop.c)
        set -- "$@" --command-name fib_loop_clang 'bench/fib_loop.exe'
        ;;
      bench/fib_loop.af)
        set -- "$@" --command-name fib_loop_astil_aot 'bench/fib_loop_astil.exe'
        set -- "$@" --command-name fib_loop_astil_reg './astil.exe bench/fib_loop.af'
        ;;
      bench/fib_asm.af)
        set -- "$@" --command-name fib_loop_astil_asm './astil.exe bench/fib_asm.af'
        ;;
      bench/fib_loop_s.af)
        set -- "$@" --command-name fib_loop_astil_stack './astil_s.exe bench/fib_loop_s.af'
        ;;
      bench/fib_loop_g.fs)
        set -- "$@" --command-name fib_loop_gforth 'gforth bench/fib_loop_g.fs -e bye'
        ;;
      bench/fib_loop.lua)
        set -- "$@" --command-name fib_loop_luajit 'luajit bench/fib_loop.lua'
        ;;
      bench/fib_loop.mjs)
        set -- "$@" --command-name fib_loop_js_bun 'bun run bench/fib_loop.mjs'
        ;;
      bench/fib_loop.lisp)
        set -- "$@" --command-name fib_loop_cl_sbcl 'sbcl --script bench/fib_loop.lisp'
        ;;
      bench/fib_loop.py)
        set -- "$@" --command-name fib_loop_pypy 'pypy3 bench/fib_loop.py'
        set -- "$@" --command-name fib_loop_python 'python3 bench/fib_loop.py'
        ;;
      bench/fib_loop_big.mjs)
        set -- "$@" --command-name fib_loop_big_js_bun 'bun run bench/fib_loop_big.mjs'
        ;;
      bench/fib_loop_big.lisp)
        set -- "$@" --command-name fib_loop_big_cl_sbcl 'sbcl --script bench/fib_loop_big.lisp'
        ;;
      bench/fib_loop_big.py)
        set -- "$@" --command-name fib_loop_big_pypy 'pypy3 bench/fib_loop_big.py'
        set -- "$@" --command-name fib_loop_big_python 'python3 bench/fib_loop_big.py'
        ;;
      bench/fib_rec.c)
        set -- "$@" --command-name fib_rec_clang 'bench/fib_rec.exe'
        ;;
      bench/fib_rec.af)
        set -- "$@" --command-name fib_rec_astil_aot 'bench/fib_rec_astil.exe'
        set -- "$@" --command-name fib_rec_astil_reg './astil.exe bench/fib_rec.af'
        ;;
      bench/fib_rec_s.af)
        set -- "$@" --command-name fib_rec_astil_stack './astil_s.exe bench/fib_rec_s.af'
        ;;
      bench/fib_rec_g.fs)
        set -- "$@" --command-name fib_rec_gforth 'gforth bench/fib_rec_g.fs -e bye'
        ;;
      bench/fib_rec.lua)
        set -- "$@" --command-name fib_rec_luajit 'luajit bench/fib_rec.lua'
        ;;
      bench/fib_rec.mjs)
        set -- "$@" --command-name fib_rec_js_bun 'bun run bench/fib_rec.mjs'
        ;;
      bench/fib_rec.lisp)
        set -- "$@" --command-name fib_rec_cl_sbcl 'sbcl --script bench/fib_rec.lisp'
        ;;
      bench/fib_rec.py)
        set -- "$@" --command-name fib_rec_pypy 'pypy3 bench/fib_rec.py'
        set -- "$@" --command-name fib_rec_python 'python3 bench/fib_rec.py'
        ;;
    esac
  done

  if [ "$#" -eq 0 ]
  then return 0
  fi

  MATCHED=true

  {
    echo
    echo "## $title"
    echo
    hyperfine $OPTS --style=none --export-markdown=/dev/stdout "$@"
  } >> "$GEN/bench.md"
}

if ! has_match
then
  echo "no benchmarks match filters:" "$@" >&2
  exit 2
fi

make clean
mkdir -p "$GEN"

{
  echo
  echo '## VERSIONS'
  echo
  echo '```'
  clang --version
  echo
  gforth --version 2>&1
  echo
  luajit -v
  echo
  bun --version
  echo
  sbcl --version
  echo
  pypy3 --version
  echo
  python3 --version
  echo '```'
} >> "$GEN/bench.md"

make PROD=true build

make PROD=true bench/bubble.exe
./astil.exe bench/bubble.af --build=bench/bubble_astil.exe

make PROD=true bench/sieve.exe
./astil.exe bench/sieve.af --build=bench/sieve_astil.exe

make PROD=true bench/fib_loop.exe
./astil.exe bench/fib_loop.af --build=bench/fib_loop_astil.exe

make PROD=true bench/fib_rec.exe
./astil.exe bench/fib_rec.af --build=bench/fib_rec_astil.exe

run_section NONE \
  astil.exe astil_s.exe

run_section BASELINE \
  forth/lang.af forth/lang_s.af gforth sbcl

run_section BUBBLE \
  bench/bubble.c bench/bubble.af bench/bubble_s.af bench/bubble_g.fs bench/bubble.lua bench/bubble.mjs bench/bubble.lisp bench/bubble.py

run_section 'PRIME SIEVE' \
  bench/sieve.c bench/sieve.af bench/sieve_s.af bench/sieve_g.fs bench/sieve.lua bench/sieve.mjs bench/sieve.lisp bench/sieve.py

run_section 'FIB_LOOP' \
  bench/fib_loop.c bench/fib_loop.af bench/fib_asm.af bench/fib_loop_s.af bench/fib_loop_g.fs bench/fib_loop.lua bench/fib_loop.mjs bench/fib_loop.lisp bench/fib_loop.py

run_section 'FIB_LOOP_BIG' \
  bench/fib_loop_big.mjs bench/fib_loop_big.lisp bench/fib_loop_big.py

run_section 'FIB_RECURSIVE: fib(36)' \
  bench/fib_rec.c bench/fib_rec.af bench/fib_rec_s.af bench/fib_rec_g.fs bench/fib_rec.lua bench/fib_rec.mjs bench/fib_rec.lisp bench/fib_rec.py

if [ "$MATCHED" = false ]
then
  echo "no benchmarks match filters:" "$@" >&2
  exit 2
fi
