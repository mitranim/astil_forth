#! /usr/bin/env sh

cd "$(realpath "$(dirname -- "$0")")/.." &&

make PROD=true clean build bench/fib_rec.exe &&

# OPTS='--warmup=4 --shell=none --show-output' &&
OPTS='--warmup=4 --shell=none' &&

echo
echo "## BASELINE"
echo
hyperfine $OPTS                                             \
  --command-name baseline_our_r  './astil_r forth/lang_r.f' \
  --command-name baseline_our_s  './astil_s forth/lang_s.f' \
  --command-name baseline_gforth 'gforth -e bye'            \

echo
echo "## BUBBLE"
echo
hyperfine $OPTS                                                  \
  --command-name bubble_our_r  './astil_r bench/bubble_r.f'      \
  --command-name bubble_our_s  './astil_s bench/bubble_s.f'      \
  --command-name bubble_gforth 'gforth bench/bubble_g.fs -e bye' \

echo
echo "## FIB_LOOP"
echo
hyperfine $OPTS                                                      \
  --command-name fib_loop_asm_r  './astil_r bench/fib_asm.f'         \
  --command-name fib_loop_our_r  './astil_r bench/fib_loop_r.f'      \
  --command-name fib_loop_our_s  './astil_s bench/fib_loop_s.f'      \
  --command-name fib_loop_gforth 'gforth bench/fib_loop_g.fs -e bye' \

echo
echo "## FIB_RECURSIVE: fib(36) single run"
echo
hyperfine $OPTS                                                    \
  --command-name fib_rec_clang  './bench/fib_rec.exe 36'           \
  --command-name fib_rec_our_r  './astil_r bench/fib_rec_r.f'      \
  --command-name fib_rec_our_s  './astil_s bench/fib_rec_s.f'      \
  --command-name fib_rec_gforth 'gforth bench/fib_rec_g.fs -e bye' \
  --command-name fib_rec_js_bun 'bun bench/fib_rec.mjs'            \
