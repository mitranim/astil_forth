Microbenchmarks comparing:
- Register-CC version of Astil Forth in AOT mode.
- Register-CC version of Astil Forth in JIT mode.
- Stack-CC version of Astil Forth in JIT mode.
- Gforth.
- C (Clang).
- JS (Bun).
- Lua (LuaJIT).
- Common Lisp (SBCL).
- Python (PyPy and CPython).

Summary: in these _very limited_ microbenchmarks, the reg-CC implementation of Astil Forth trounces VM interpreters, slightly outpaces other JITs (due to being untyped), and vaguely approaches Clang C with `-O2` (x1.5-x2).

Naming:
- `_aot`   -- Astil reg-CC as AOT-compiled executable.
- `_reg`   -- Astil reg-CC in JIT mode.
- `_stack` -- Astil stack-CC in JIT mode.

Each `_reg` and `_stack` benchmark includes the cost of bootstrapping the entire language first. Reg-CC takes longer to bootstrap because it has more features.

In many benchmarks, _startup time skews the measurement_. Adjust them by the "baseline" metrics when comparing.

All measurements were done on M3 Pro.

## VERSIONS

```
clang 22.1.4

gforth 0.7.3

LuaJIT 2.1.1753364724

Bun 1.3.10

SBCL 2.6.4

[PyPy 7.3.17 with GCC Apple LLVM 16.0.0 (clang-1600.0.26.4)]
Python 3.10.14 (39dc8d3c85a7, Nov 09 2024, 22:49:03)

Python 3.14.4
```

## NONE

| Command | Mean [µs] | Min [µs] | Max [µs] | Relative |
|:---|---:|---:|---:|---:|
| `none_astil_reg` | 992.2 ± 61.8 | 846.3 | 2331.8 | 1.02 ± 0.07 |
| `none_astil_stack` | 977.5 ± 28.5 | 846.5 | 1244.4 | 1.00 |

## BASELINE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `baseline_astil_reg` | 6.8 ± 0.3 | 6.4 | 8.5 | 3.57 ± 0.50 |
| `baseline_astil_stack` | 4.4 ± 0.3 | 4.0 | 6.3 | 2.30 ± 0.34 |
| `baseline_gforth` | 2.8 ± 0.2 | 2.5 | 5.6 | 1.48 ± 0.23 |
| `baseline_luajit` | 1.9 ± 0.3 | 1.3 | 2.7 | 1.00 |
| `baseline_js_bun` | 3.2 ± 0.2 | 2.7 | 4.2 | 1.70 ± 0.24 |
| `baseline_cl_sbcl` | 12.0 ± 0.8 | 10.8 | 15.0 | 6.31 ± 0.94 |
| `baseline_pypy` | 14.7 ± 0.9 | 14.1 | 25.6 | 7.72 ± 1.13 |
| `baseline_python` | 16.5 ± 1.3 | 15.2 | 21.4 | 8.65 ± 1.33 |

## BUBBLE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `bubble_clang` | 19.5 ± 0.1 | 19.4 | 20.0 | 1.00 |
| `bubble_astil_aot` | 33.5 ± 0.3 | 33.3 | 35.9 | 1.72 ± 0.02 |
| `bubble_astil_reg` | 39.2 ± 0.2 | 39.0 | 40.1 | 2.01 ± 0.01 |
| `bubble_astil_stack` | 101.4 ± 0.1 | 101.3 | 101.6 | 5.22 ± 0.03 |
| `bubble_gforth` | 216.2 ± 9.3 | 205.1 | 228.8 | 11.11 ± 0.48 |
| `bubble_luajit` | 39.0 ± 1.3 | 37.3 | 43.7 | 2.00 ± 0.07 |
| `bubble_js_bun` | 38.4 ± 0.2 | 38.0 | 38.9 | 1.97 ± 0.01 |
| `bubble_cl_sbcl` | 44.6 ± 0.2 | 43.9 | 45.7 | 2.29 ± 0.02 |
| `bubble_pypy` | 87.9 ± 0.3 | 87.4 | 88.8 | 4.52 ± 0.03 |
| `bubble_python` | 1745.8 ± 29.0 | 1720.4 | 1802.9 | 89.76 ± 1.55 |

## PRIME SIEVE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `sieve_clang` | 37.4 ± 0.3 | 37.1 | 39.4 | 1.00 |
| `sieve_astil_aot` | 62.7 ± 0.4 | 62.2 | 63.9 | 1.68 ± 0.02 |
| `sieve_astil_reg` | 68.3 ± 0.3 | 67.9 | 69.8 | 1.83 ± 0.02 |
| `sieve_astil_stack` | 182.8 ± 2.1 | 180.9 | 187.7 | 4.89 ± 0.07 |
| `sieve_gforth` | 447.8 ± 5.4 | 440.1 | 459.4 | 11.98 ± 0.18 |
| `sieve_luajit` | 65.6 ± 0.3 | 65.0 | 66.5 | 1.76 ± 0.02 |
| `sieve_js_bun` | 64.7 ± 0.4 | 63.7 | 66.3 | 1.73 ± 0.02 |
| `sieve_cl_sbcl` | 71.3 ± 0.4 | 70.8 | 72.6 | 1.91 ± 0.02 |
| `sieve_pypy` | 164.8 ± 0.5 | 164.1 | 166.0 | 4.41 ± 0.04 |
| `sieve_python` | 3917.7 ± 9.6 | 3902.0 | 3933.8 | 104.83 ± 0.90 |

## REVERSE STRING

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `reverse_string_clang` | 14.5 ± 2.0 | 12.7 | 28.8 | 1.00 |
| `reverse_string_astil_aot` | 30.0 ± 1.3 | 28.6 | 39.6 | 2.07 ± 0.30 |
| `reverse_string_astil_reg` | 35.9 ± 2.1 | 34.5 | 53.8 | 2.48 ± 0.37 |
| `reverse_string_astil_stack` | 240.1 ± 7.9 | 232.6 | 251.0 | 16.60 ± 2.32 |
| `reverse_string_gforth` | 527.3 ± 1.1 | 525.6 | 529.3 | 36.45 ± 4.94 |
| `reverse_string_luajit` | 43.6 ± 2.1 | 42.1 | 59.2 | 3.01 ± 0.43 |
| `reverse_string_js_bun` | 36.8 ± 1.4 | 35.4 | 44.7 | 2.54 ± 0.36 |
| `reverse_string_cl_sbcl` | 100.3 ± 3.8 | 97.4 | 116.8 | 6.93 ± 0.98 |
| `reverse_string_pypy` | 133.0 ± 4.8 | 129.8 | 153.8 | 9.20 ± 1.29 |
| `reverse_string_python` | 1665.9 ± 12.0 | 1645.3 | 1681.5 | 115.15 ± 15.64 |

## FIB_LOOP

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_clang` | 2.8 ± 0.0 | 2.7 | 3.1 | 1.00 |
| `fib_loop_astil_aot` | 3.5 ± 0.0 | 3.4 | 3.7 | 1.23 ± 0.02 |
| `fib_loop_astil_reg` | 9.2 ± 0.0 | 9.1 | 9.6 | 3.22 ± 0.04 |
| `fib_loop_astil_asm` | 8.4 ± 0.1 | 8.3 | 8.7 | 2.93 ± 0.04 |
| `fib_loop_astil_stack` | 23.4 ± 0.8 | 22.3 | 25.6 | 8.21 ± 0.30 |
| `fib_loop_gforth` | 41.8 ± 1.2 | 39.8 | 44.2 | 14.67 ± 0.45 |
| `fib_loop_luajit` | 9.2 ± 0.1 | 8.3 | 9.5 | 3.24 ± 0.04 |
| `fib_loop_js_bun` | 13.2 ± 0.3 | 12.1 | 13.9 | 4.64 ± 0.12 |
| `fib_loop_cl_sbcl` | 15.8 ± 0.2 | 15.2 | 16.8 | 5.55 ± 0.10 |
| `fib_loop_pypy` | 29.7 ± 1.2 | 25.5 | 31.1 | 10.42 ± 0.44 |
| `fib_loop_python` | 222.8 ± 1.1 | 221.3 | 224.8 | 78.18 ± 0.91 |

## FIB_LOOP_BIG

C and Astil benchmarks use uint128. JS/CL/Python use actual bigints.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_big_clang` | 16.9 ± 1.3 | 15.2 | 20.4 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 17.0 ± 0.4 | 16.5 | 19.2 | 1.01 ± 0.08 |
| `fib_loop_big_astil_asm_reg` | 23.3 ± 0.5 | 22.6 | 24.8 | 1.38 ± 0.11 |
| `fib_loop_big_astil_aot` | 63.1 ± 1.6 | 61.2 | 65.6 | 3.73 ± 0.31 |
| `fib_loop_big_astil_reg` | 69.5 ± 1.3 | 68.0 | 71.8 | 4.11 ± 0.33 |
| `fib_loop_big_js_bun` | 415.1 ± 1.9 | 412.2 | 417.7 | 24.54 ± 1.94 |
| `fib_loop_big_cl_sbcl` | 330.1 ± 0.7 | 329.0 | 330.9 | 19.52 ± 1.54 |
| `fib_loop_big_pypy` | 358.8 ± 1.6 | 356.3 | 362.3 | 21.22 ± 1.68 |
| `fib_loop_big_python` | 1719.9 ± 5.6 | 1710.3 | 1729.1 | 101.70 ± 8.04 |

## FIB_RECURSIVE: fib(36)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_rec_clang` | 39.3 ± 0.7 | 38.3 | 42.1 | 1.00 |
| `fib_rec_astil_aot` | 57.9 ± 0.6 | 56.8 | 59.6 | 1.47 ± 0.03 |
| `fib_rec_astil_reg` | 63.4 ± 0.4 | 62.6 | 64.3 | 1.61 ± 0.03 |
| `fib_rec_astil_stack` | 265.4 ± 11.8 | 257.0 | 286.6 | 6.75 ± 0.32 |
| `fib_rec_gforth` | 342.9 ± 6.1 | 332.7 | 350.9 | 8.72 ± 0.22 |
| `fib_rec_luajit` | 69.4 ± 3.0 | 66.3 | 78.3 | 1.77 ± 0.08 |
| `fib_rec_js_bun` | 69.1 ± 0.8 | 67.8 | 72.5 | 1.76 ± 0.04 |
| `fib_rec_cl_sbcl` | 91.3 ± 0.3 | 90.9 | 92.3 | 2.32 ± 0.04 |
| `fib_rec_pypy` | 139.1 ± 0.3 | 138.5 | 139.7 | 3.54 ± 0.06 |
| `fib_rec_python` | 1592.0 ± 14.0 | 1571.2 | 1617.5 | 40.50 ± 0.80 |
