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

All measurements were done on M3 Pro.

Summary: in these _very limited_ microbenchmarks, the reg-CC implementation of Astil Forth trounces VM interpreters, approximates other JITs, and vaguely approaches Clang C with `-O2` (x1.5-x2).

Caveat: CPU microbenchmarks are sensitive to code layout and instruction selection. Small source changes can shift CPU frontend, cache, and branch-prediction behavior; on M3 Pro, we have seen cosmetic-looking benchmark edits move results by up to ≈10%. Avoid over-generalizing differences in that range.

Naming:
- `_aot`     -- Astil reg-CC as AOT-compiled executable.
- `_reg`     -- Astil reg-CC in JIT mode.
- `_asm_aot` -- Astil reg-CC in AOT mode.
- `_stack`   -- Astil stack-CC in JIT mode.

Each `_reg` and `_stack` benchmark includes the cost of bootstrapping the entire language first. Reg-CC takes longer to bootstrap because it has more features.

In many benchmarks, _startup time skews the measurement_. Adjust them by the "baseline" metrics when comparing.

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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `none_astil_stack` | 1.0 ± 0.1 | 0.8 | 1.4 | 1.00 |
| `none_astil_reg` | 1.1 ± 0.2 | 0.8 | 2.2 | 1.10 ± 0.18 |

## BASELINE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `baseline_luajit` | 2.3 ± 0.5 | 1.5 | 3.7 | 1.00 |
| `baseline_astil_stack` | 2.9 ± 0.1 | 2.7 | 3.3 | 1.23 ± 0.28 |
| `baseline_gforth` | 3.4 ± 0.5 | 2.4 | 4.9 | 1.44 ± 0.40 |
| `baseline_js_bun` | 3.7 ± 0.5 | 2.8 | 4.8 | 1.59 ± 0.41 |
| `baseline_astil_reg` | 10.6 ± 0.2 | 10.2 | 11.2 | 4.53 ± 1.03 |
| `baseline_cl_sbcl` | 12.0 ± 0.6 | 10.7 | 13.5 | 5.14 ± 1.19 |
| `baseline_pypy` | 14.8 ± 0.5 | 13.7 | 16.2 | 6.34 ± 1.45 |
| `baseline_python` | 16.0 ± 0.4 | 14.9 | 17.2 | 6.86 ± 1.56 |

## BUBBLE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `bubble_clang` | 18.1 ± 0.2 | 17.9 | 19.0 | 1.00 |
| `bubble_astil_aot` | 29.9 ± 0.2 | 29.5 | 30.7 | 1.65 ± 0.02 |
| `bubble_js_bun` | 37.0 ± 0.9 | 35.6 | 39.0 | 2.04 ± 0.05 |
| `bubble_astil_reg` | 38.6 ± 1.7 | 36.0 | 41.3 | 2.13 ± 0.10 |
| `bubble_luajit` | 38.8 ± 1.2 | 36.6 | 43.4 | 2.14 ± 0.07 |
| `bubble_cl_sbcl` | 43.3 ± 0.9 | 41.4 | 45.6 | 2.39 ± 0.06 |
| `bubble_pypy` | 87.2 ± 0.5 | 86.2 | 88.1 | 4.81 ± 0.06 |
| `bubble_astil_stack` | 97.7 ± 1.4 | 95.5 | 99.6 | 5.39 ± 0.09 |
| `bubble_gforth` | 212.9 ± 7.6 | 200.3 | 226.0 | 11.75 ± 0.44 |
| `bubble_python` | 1695.6 ± 14.0 | 1675.7 | 1719.5 | 93.54 ± 1.23 |

## PRIME SIEVE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `sieve_clang` | 36.8 ± 0.3 | 36.5 | 37.8 | 1.00 |
| `sieve_cl_sbcl` | 55.8 ± 1.3 | 53.8 | 58.6 | 1.51 ± 0.04 |
| `sieve_astil_aot` | 56.1 ± 1.8 | 54.3 | 59.0 | 1.52 ± 0.05 |
| `sieve_js_bun` | 61.1 ± 1.1 | 59.5 | 63.6 | 1.66 ± 0.03 |
| `sieve_luajit` | 64.4 ± 0.6 | 63.0 | 65.3 | 1.75 ± 0.02 |
| `sieve_astil_reg` | 66.5 ± 2.1 | 64.2 | 69.3 | 1.81 ± 0.06 |
| `sieve_pypy` | 162.8 ± 0.5 | 161.9 | 163.6 | 4.42 ± 0.03 |
| `sieve_astil_stack` | 177.3 ± 0.7 | 176.6 | 178.8 | 4.81 ± 0.04 |
| `sieve_gforth` | 441.6 ± 2.7 | 437.2 | 445.9 | 11.99 ± 0.11 |
| `sieve_python` | 3861.2 ± 12.5 | 3844.2 | 3886.2 | 104.80 ± 0.81 |

## REVERSE STRING

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `reverse_string_clang` | 13.2 ± 0.9 | 12.7 | 15.9 | 1.00 |
| `reverse_string_astil_aot` | 29.8 ± 0.4 | 29.0 | 32.0 | 2.26 ± 0.16 |
| `reverse_string_js_bun` | 37.0 ± 0.4 | 36.2 | 37.9 | 2.80 ± 0.20 |
| `reverse_string_astil_reg` | 40.1 ± 0.4 | 39.2 | 40.9 | 3.03 ± 0.21 |
| `reverse_string_luajit` | 43.3 ± 0.7 | 41.9 | 46.5 | 3.27 ± 0.23 |
| `reverse_string_cl_sbcl` | 100.3 ± 2.7 | 97.4 | 105.6 | 7.58 ± 0.57 |
| `reverse_string_pypy` | 132.2 ± 1.3 | 130.2 | 134.8 | 10.00 ± 0.71 |
| `reverse_string_astil_stack` | 243.7 ± 1.9 | 241.0 | 247.2 | 18.44 ± 1.30 |
| `reverse_string_gforth` | 525.9 ± 1.3 | 523.8 | 527.6 | 39.78 ± 2.78 |
| `reverse_string_python` | 1685.0 ± 5.9 | 1676.0 | 1693.6 | 127.47 ± 8.91 |

## FIB_LOOP

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_astil_asm_aot` | 26.6 ± 0.2 | 26.2 | 27.3 | 1.00 |
| `fib_loop_clang` | 30.3 ± 0.3 | 29.6 | 31.6 | 1.14 ± 0.01 |
| `fib_loop_js_bun` | 37.0 ± 0.5 | 35.9 | 37.9 | 1.39 ± 0.02 |
| `fib_loop_astil_aot` | 41.3 ± 0.6 | 40.3 | 42.5 | 1.56 ± 0.03 |
| `fib_loop_astil_reg` | 54.4 ± 3.1 | 50.0 | 60.1 | 2.05 ± 0.12 |
| `fib_loop_cl_sbcl` | 59.3 ± 1.1 | 57.2 | 62.0 | 2.23 ± 0.05 |
| `fib_loop_luajit` | 95.6 ± 0.5 | 94.0 | 96.6 | 3.60 ± 0.04 |
| `fib_loop_pypy` | 215.1 ± 0.9 | 213.5 | 216.6 | 8.10 ± 0.08 |
| `fib_loop_astil_stack` | 296.9 ± 16.5 | 278.3 | 335.9 | 11.18 ± 0.63 |
| `fib_loop_gforth` | 604.4 ± 17.8 | 582.5 | 623.6 | 22.76 ± 0.70 |
| `fib_loop_python` | 3232.3 ± 12.3 | 3223.0 | 3256.0 | 121.70 ± 1.23 |

## FIB_LOOP_BIG

C and Astil benchmarks use uint128. CL/JS/Python use actual bigints.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_big_clang` | 15.7 ± 0.4 | 15.0 | 16.5 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 17.5 ± 0.5 | 16.4 | 18.3 | 1.11 ± 0.04 |
| `fib_loop_big_astil_asm_reg` | 27.7 ± 0.7 | 26.2 | 28.6 | 1.77 ± 0.06 |
| `fib_loop_big_astil_aot` | 64.7 ± 0.7 | 63.2 | 65.5 | 4.12 ± 0.12 |
| `fib_loop_big_astil_reg` | 75.0 ± 0.6 | 73.5 | 75.8 | 4.78 ± 0.13 |
| `fib_loop_big_cl_sbcl` | 263.1 ± 0.6 | 262.0 | 264.0 | 16.77 ± 0.44 |
| `fib_loop_big_pypy` | 360.8 ± 2.0 | 359.2 | 366.2 | 22.99 ± 0.61 |
| `fib_loop_big_js_bun` | 419.5 ± 2.9 | 416.0 | 425.5 | 26.73 ± 0.72 |
| `fib_loop_big_python` | 1708.8 ± 2.8 | 1704.4 | 1714.2 | 108.90 ± 2.85 |

## FIB_RECURSIVE: fib(36)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_rec_clang` | 37.8 ± 1.0 | 35.4 | 39.4 | 1.00 |
| `fib_rec_astil_aot` | 56.5 ± 1.1 | 54.4 | 61.1 | 1.50 ± 0.05 |
| `fib_rec_luajit` | 67.9 ± 2.2 | 64.8 | 75.0 | 1.79 ± 0.08 |
| `fib_rec_astil_reg` | 69.3 ± 4.8 | 65.0 | 80.5 | 1.83 ± 0.14 |
| `fib_rec_js_bun` | 71.1 ± 3.6 | 67.0 | 88.1 | 1.88 ± 0.11 |
| `fib_rec_cl_sbcl` | 89.1 ± 1.5 | 87.1 | 92.1 | 2.36 ± 0.07 |
| `fib_rec_pypy` | 139.0 ± 2.2 | 136.2 | 144.9 | 3.68 ± 0.11 |
| `fib_rec_astil_stack` | 248.8 ± 7.3 | 236.1 | 255.5 | 6.58 ± 0.26 |
| `fib_rec_gforth` | 341.4 ± 2.5 | 339.5 | 347.6 | 9.03 ± 0.25 |
| `fib_rec_python` | 1572.2 ± 14.8 | 1552.8 | 1595.8 | 41.58 ± 1.18 |

## CONST FOLD

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `const_fold_folded_astil_aot` | 9.5 ± 0.3 | 9.2 | 10.2 | 1.00 |
| `const_fold_folded_astil_reg` | 19.8 ± 0.5 | 18.9 | 21.7 | 2.08 ± 0.08 |
| `const_fold_runtime_astil_aot` | 174.5 ± 1.2 | 173.5 | 177.9 | 18.30 ± 0.52 |
| `const_fold_runtime_astil_reg` | 185.7 ± 1.6 | 183.9 | 189.3 | 19.48 ± 0.57 |

## FNV-1A 64

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fnv1a64_clang` | 35.8 ± 1.0 | 34.4 | 37.5 | 1.00 |
| `fnv1a64_astil_aot` | 36.4 ± 0.7 | 34.8 | 37.6 | 1.02 ± 0.04 |
| `fnv1a64_astil_reg` | 46.7 ± 0.8 | 45.0 | 48.6 | 1.30 ± 0.04 |
| `fnv1a64_cl_sbcl` | 48.2 ± 1.3 | 46.0 | 51.1 | 1.35 ± 0.05 |
| `fnv1a64_pypy` | 83.8 ± 0.6 | 82.1 | 85.4 | 2.34 ± 0.07 |
| `fnv1a64_gforth` | 135.8 ± 3.0 | 128.3 | 141.2 | 3.79 ± 0.14 |
| `fnv1a64_astil_stack` | 166.3 ± 16.7 | 150.0 | 210.1 | 4.65 ± 0.49 |
| `fnv1a64_luajit` | 292.2 ± 6.8 | 279.3 | 296.4 | 8.16 ± 0.30 |
| `fnv1a64_js_bun` | 411.8 ± 9.4 | 399.8 | 424.7 | 11.50 ± 0.42 |
| `fnv1a64_python` | 2473.3 ± 15.0 | 2457.5 | 2501.2 | 69.10 ± 2.04 |

## SCAN DELIMS

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `scan_delims_c_simd` | 19.0 ± 0.2 | 18.6 | 19.5 | 1.00 |
| `scan_delims_astil_simd_aot` | 19.3 ± 0.2 | 18.9 | 20.0 | 1.02 ± 0.02 |
| `scan_delims_astil_simd_reg` | 30.3 ± 0.3 | 29.7 | 31.2 | 1.60 ± 0.02 |
| `scan_delims_c_naive` | 51.3 ± 0.4 | 50.8 | 52.5 | 2.70 ± 0.04 |
| `scan_delims_luajit` | 86.8 ± 0.7 | 85.5 | 87.9 | 4.57 ± 0.06 |
| `scan_delims_js_bun` | 136.4 ± 0.4 | 135.4 | 136.9 | 7.19 ± 0.08 |
| `scan_delims_astil_naive_reg` | 146.8 ± 0.2 | 146.5 | 147.3 | 7.74 ± 0.08 |
| `scan_delims_cl_sbcl` | 160.3 ± 0.7 | 158.6 | 161.1 | 8.45 ± 0.10 |
| `scan_delims_pypy` | 193.9 ± 0.6 | 192.2 | 194.5 | 10.22 ± 0.11 |
| `scan_delims_python` | 661.9 ± 2.0 | 658.9 | 664.5 | 34.88 ± 0.38 |
