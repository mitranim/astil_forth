#! /usr/bin/env sh

# Non-zero exit = exception.
set -e

# Ensure we're in repo root.
cd "$(realpath "$(dirname -- "$0")")/.."

make PROD=true clean build

# OPTS='--warmup=8 --shell=none --show-output'
OPTS='--warmup=8 --shell=none'

echo
echo "## NONE"
echo
hyperfine $OPTS                             \
  --command-name none_reg   './astil.exe'   \
  --command-name none_stack './astil_s.exe'

echo
echo "## BASELINE"
echo
hyperfine $OPTS                                                       \
  --command-name baseline_reg         './astil.exe   forth/lang.af'   \
  --command-name baseline_stack       './astil_s.exe forth/lang_s.af' \
  --command-name baseline_gforth      'gforth -e bye'                 \
  --command-name baseline_gforth_fast 'gforth-fast -e bye'            \

./astil.exe bench/bubble.af --build=bench/bubble.exe

echo
echo "## BUBBLE"
echo
hyperfine $OPTS                                                            \
  --command-name bubble_aot         'bench/bubble.exe'                     \
  --command-name bubble_reg         './astil.exe   bench/bubble.af'        \
  --command-name bubble_stack       './astil_s.exe bench/bubble_s.af'      \
  --command-name bubble_gforth      'gforth bench/bubble_g.fs -e bye'      \
  --command-name bubble_gforth_fast 'gforth-fast bench/bubble_g.fs -e bye' \

./astil.exe bench/sieve.af --build=bench/sieve.exe

echo
echo "## PRIME SIEVE"
echo
hyperfine $OPTS                                                          \
  --command-name sieve_aot         'bench/sieve.exe'                     \
  --command-name sieve_reg         './astil.exe   bench/sieve.af'        \
  --command-name sieve_stack       './astil_s.exe bench/sieve_s.af'      \
  --command-name sieve_gforth      'gforth bench/sieve_g.fs -e bye'      \
  --command-name sieve_gforth_fast 'gforth-fast bench/sieve_g.fs -e bye' \

./astil.exe bench/fib_loop.af --build=bench/fib_loop.exe

echo
echo "## FIB_LOOP"
echo
hyperfine $OPTS                                                                \
  --command-name fib_loop_aot         'bench/fib_loop.exe'                     \
  --command-name fib_loop_asm         './astil.exe   bench/fib_asm.af'         \
  --command-name fib_loop_reg         './astil.exe   bench/fib_loop.af'        \
  --command-name fib_loop_stack       './astil_s.exe bench/fib_loop_s.af'      \
  --command-name fib_loop_gforth      'gforth bench/fib_loop_g.fs -e bye'      \
  --command-name fib_loop_gforth_fast 'gforth-fast bench/fib_loop_g.fs -e bye' \

make PROD=true bench/fib_rec.exe
./astil.exe bench/fib_rec.af --build=bench/fib_rec_aot.exe

echo
echo "## FIB_RECURSIVE: fib(36) single run"
echo
hyperfine $OPTS                                                              \
  --command-name fib_rec_clang       'bench/fib_rec.exe'                     \
  --command-name fib_rec_aot         'bench/fib_rec_aot.exe'                 \
  --command-name fib_rec_reg         './astil.exe bench/fib_rec.af'          \
  --command-name fib_rec_stack       './astil_s.exe bench/fib_rec_s.af'      \
  --command-name fib_rec_gforth      'gforth bench/fib_rec_g.fs -e bye'      \
  --command-name fib_rec_gforth_fast 'gforth-fast bench/fib_rec_g.fs -e bye' \
  --command-name fib_rec_luajit      'luajit bench/fib_rec.lua'              \
  --command-name fib_rec_js_bun      'bun bench/fib_rec.mjs'                 \
