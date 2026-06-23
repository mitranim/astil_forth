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
| `none_astil_stack` | 1.0 ± 0.0 | 0.8 | 1.2 | 1.00 |
| `none_astil_reg` | 1.1 ± 0.2 | 0.8 | 5.3 | 1.13 ± 0.22 |

## BASELINE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `baseline_luajit` | 2.4 ± 0.5 | 1.5 | 3.4 | 1.00 |
| `baseline_astil_stack` | 2.9 ± 0.2 | 2.7 | 6.2 | 1.20 ± 0.26 |
| `baseline_gforth` | 3.6 ± 0.5 | 2.4 | 4.4 | 1.47 ± 0.36 |
| `baseline_js_bun` | 3.7 ± 0.4 | 2.7 | 4.7 | 1.54 ± 0.37 |
| `baseline_cl_sbcl` | 12.3 ± 0.6 | 10.7 | 14.3 | 5.05 ± 1.08 |
| `baseline_astil_reg` | 12.8 ± 0.3 | 12.3 | 13.5 | 5.24 ± 1.10 |
| `baseline_pypy` | 15.1 ± 0.6 | 14.0 | 16.9 | 6.21 ± 1.32 |
| `baseline_python` | 16.1 ± 0.5 | 14.5 | 17.5 | 6.62 ± 1.40 |

## BUBBLE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `bubble_clang` | 18.4 ± 0.4 | 17.9 | 19.4 | 1.00 |
| `bubble_astil_aot` | 31.5 ± 0.7 | 30.4 | 32.8 | 1.71 ± 0.06 |
| `bubble_js_bun` | 37.3 ± 0.8 | 35.7 | 38.8 | 2.03 ± 0.07 |
| `bubble_luajit` | 38.5 ± 1.0 | 36.5 | 41.9 | 2.10 ± 0.07 |
| `bubble_astil_reg` | 44.0 ± 0.9 | 42.4 | 45.7 | 2.39 ± 0.08 |
| `bubble_cl_sbcl` | 44.2 ± 0.7 | 42.3 | 45.7 | 2.40 ± 0.07 |
| `bubble_pypy` | 87.5 ± 0.5 | 86.5 | 88.4 | 4.76 ± 0.11 |
| `bubble_astil_stack` | 98.8 ± 0.8 | 97.5 | 99.8 | 5.37 ± 0.13 |
| `bubble_gforth` | 216.6 ± 9.2 | 200.6 | 231.6 | 11.78 ± 0.57 |
| `bubble_python` | 1686.9 ± 16.4 | 1676.4 | 1732.1 | 91.72 ± 2.30 |

## PRIME SIEVE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `sieve_clang` | 37.0 ± 0.2 | 36.6 | 37.5 | 1.00 |
| `sieve_astil_aot` | 54.3 ± 0.7 | 53.1 | 56.2 | 1.47 ± 0.02 |
| `sieve_js_bun` | 62.3 ± 1.8 | 59.8 | 65.0 | 1.69 ± 0.05 |
| `sieve_luajit` | 64.8 ± 0.5 | 63.2 | 65.5 | 1.75 ± 0.02 |
| `sieve_astil_reg` | 66.9 ± 0.8 | 65.5 | 68.3 | 1.81 ± 0.02 |
| `sieve_cl_sbcl` | 70.4 ± 1.7 | 66.6 | 73.0 | 1.90 ± 0.05 |
| `sieve_pypy` | 162.9 ± 0.7 | 161.7 | 164.4 | 4.40 ± 0.03 |
| `sieve_astil_stack` | 179.7 ± 2.5 | 176.3 | 184.5 | 4.86 ± 0.07 |
| `sieve_gforth` | 442.8 ± 4.6 | 436.5 | 452.3 | 11.97 ± 0.14 |
| `sieve_python` | 3848.3 ± 8.1 | 3834.7 | 3866.3 | 104.06 ± 0.56 |

## REVERSE STRING

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `reverse_string_clang` | 13.2 ± 0.8 | 12.7 | 16.0 | 1.00 |
| `reverse_string_astil_aot` | 30.6 ± 0.2 | 30.2 | 31.2 | 2.32 ± 0.15 |
| `reverse_string_js_bun` | 37.0 ± 0.5 | 35.8 | 37.8 | 2.81 ± 0.18 |
| `reverse_string_astil_reg` | 43.1 ± 0.3 | 42.5 | 44.0 | 3.27 ± 0.21 |
| `reverse_string_luajit` | 43.7 ± 1.1 | 42.1 | 47.6 | 3.32 ± 0.23 |
| `reverse_string_cl_sbcl` | 101.1 ± 2.4 | 98.0 | 105.0 | 7.67 ± 0.52 |
| `reverse_string_pypy` | 132.0 ± 1.2 | 130.4 | 134.6 | 10.02 ± 0.64 |
| `reverse_string_astil_stack` | 244.8 ± 1.6 | 242.7 | 247.3 | 18.58 ± 1.18 |
| `reverse_string_gforth` | 527.5 ± 1.7 | 525.3 | 530.6 | 40.03 ± 2.54 |
| `reverse_string_python` | 1685.2 ± 5.6 | 1677.4 | 1696.4 | 127.90 ± 8.12 |

## FIB_LOOP

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_astil_asm_aot` | 30.4 ± 0.6 | 29.5 | 32.6 | 1.00 |
| `fib_loop_clang` | 30.8 ± 0.5 | 30.0 | 31.8 | 1.01 ± 0.02 |
| `fib_loop_js_bun` | 36.9 ± 0.5 | 35.5 | 38.0 | 1.21 ± 0.03 |
| `fib_loop_astil_aot` | 52.5 ± 0.7 | 51.1 | 53.7 | 1.73 ± 0.04 |
| `fib_loop_cl_sbcl` | 59.6 ± 0.8 | 57.5 | 60.9 | 1.96 ± 0.05 |
| `fib_loop_astil_reg` | 63.3 ± 3.3 | 55.5 | 65.9 | 2.08 ± 0.12 |
| `fib_loop_luajit` | 95.7 ± 0.5 | 94.2 | 96.6 | 3.14 ± 0.06 |
| `fib_loop_pypy` | 199.8 ± 30.9 | 144.7 | 216.9 | 6.57 ± 1.02 |
| `fib_loop_astil_stack` | 295.1 ± 11.1 | 280.8 | 314.5 | 9.70 ± 0.41 |
| `fib_loop_gforth` | 576.2 ± 10.6 | 564.4 | 597.8 | 18.95 ± 0.50 |
| `fib_loop_python` | 3226.3 ± 6.3 | 3213.8 | 3235.7 | 106.08 ± 2.00 |

## FIB_LOOP_BIG

C and Astil benchmarks use uint128. CL/JS/Python use actual bigints.

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_big_clang` | 15.6 ± 0.4 | 15.0 | 16.7 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 17.0 ± 0.4 | 16.2 | 17.9 | 1.09 ± 0.04 |
| `fib_loop_big_astil_asm_reg` | 29.6 ± 0.5 | 28.5 | 30.5 | 1.90 ± 0.06 |
| `fib_loop_big_astil_aot` | 65.3 ± 0.3 | 64.8 | 66.0 | 4.19 ± 0.12 |
| `fib_loop_big_astil_reg` | 77.7 ± 0.2 | 77.1 | 78.2 | 4.99 ± 0.14 |
| `fib_loop_big_cl_sbcl` | 333.5 ± 1.9 | 331.9 | 338.3 | 21.41 ± 0.62 |
| `fib_loop_big_pypy` | 359.4 ± 1.3 | 358.1 | 362.5 | 23.08 ± 0.66 |
| `fib_loop_big_js_bun` | 425.4 ± 14.0 | 416.6 | 463.3 | 27.31 ± 1.19 |
| `fib_loop_big_python` | 1708.7 ± 6.0 | 1703.7 | 1724.8 | 109.71 ± 3.15 |

## FIB_RECURSIVE: fib(36)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_rec_clang` | 37.1 ± 1.1 | 35.7 | 39.5 | 1.00 |
| `fib_rec_astil_aot` | 57.1 ± 1.1 | 55.8 | 62.1 | 1.54 ± 0.05 |
| `fib_rec_luajit` | 67.9 ± 1.9 | 65.2 | 75.6 | 1.83 ± 0.08 |
| `fib_rec_js_bun` | 68.8 ± 1.1 | 67.1 | 72.2 | 1.85 ± 0.06 |
| `fib_rec_astil_reg` | 69.4 ± 0.5 | 68.2 | 70.7 | 1.87 ± 0.06 |
| `fib_rec_cl_sbcl` | 89.5 ± 1.0 | 87.8 | 91.6 | 2.41 ± 0.08 |
| `fib_rec_pypy` | 137.7 ± 0.7 | 135.8 | 138.7 | 3.71 ± 0.11 |
| `fib_rec_astil_stack` | 251.2 ± 6.6 | 239.6 | 262.3 | 6.77 ± 0.27 |
| `fib_rec_gforth` | 340.7 ± 5.1 | 331.1 | 345.6 | 9.18 ± 0.31 |
| `fib_rec_python` | 1562.9 ± 15.8 | 1540.2 | 1581.9 | 42.11 ± 1.33 |

## CONST FOLD

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `const_fold_folded_astil_aot` | 9.2 ± 0.2 | 8.9 | 9.8 | 1.00 |
| `const_fold_folded_astil_reg` | 21.8 ± 0.2 | 21.2 | 22.3 | 2.36 ± 0.05 |
| `const_fold_runtime_astil_aot` | 168.5 ± 0.5 | 167.2 | 169.0 | 18.24 ± 0.36 |
| `const_fold_runtime_astil_reg` | 180.1 ± 0.6 | 178.7 | 180.8 | 19.50 ± 0.39 |

## FNV-1A 64

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fnv1a64_clang` | 34.7 ± 0.4 | 34.3 | 37.4 | 1.00 |
| `fnv1a64_astil_aot` | 38.6 ± 1.0 | 37.3 | 43.1 | 1.11 ± 0.03 |
| `fnv1a64_cl_sbcl` | 48.5 ± 1.3 | 46.6 | 51.4 | 1.40 ± 0.04 |
| `fnv1a64_astil_reg` | 51.0 ± 0.6 | 49.8 | 52.0 | 1.47 ± 0.02 |
| `fnv1a64_pypy` | 83.8 ± 0.5 | 82.2 | 84.6 | 2.41 ± 0.03 |
| `fnv1a64_gforth` | 135.2 ± 2.6 | 131.9 | 142.7 | 3.90 ± 0.09 |
| `fnv1a64_astil_stack` | 166.5 ± 12.7 | 154.2 | 207.0 | 4.80 ± 0.37 |
| `fnv1a64_luajit` | 281.0 ± 5.8 | 278.5 | 297.4 | 8.10 ± 0.19 |
| `fnv1a64_js_bun` | 406.8 ± 7.6 | 399.6 | 419.2 | 11.73 ± 0.26 |
| `fnv1a64_python` | 2469.2 ± 16.3 | 2448.8 | 2501.0 | 71.17 ± 1.00 |

## SCAN DELIMS

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `scan_delims_c_simd` | 18.9 ± 0.2 | 18.6 | 19.5 | 1.00 |
| `scan_delims_astil_simd_aot` | 19.1 ± 0.2 | 18.8 | 19.7 | 1.01 ± 0.01 |
| `scan_delims_astil_simd_reg` | 32.4 ± 0.2 | 31.8 | 32.9 | 1.71 ± 0.02 |
| `scan_delims_c_naive` | 51.0 ± 0.2 | 50.7 | 52.3 | 2.70 ± 0.03 |
| `scan_delims_luajit` | 86.7 ± 0.6 | 85.6 | 87.4 | 4.59 ± 0.05 |
| `scan_delims_js_bun` | 136.4 ± 0.4 | 135.8 | 137.5 | 7.23 ± 0.07 |
| `scan_delims_astil_naive_reg` | 151.4 ± 0.2 | 151.1 | 151.8 | 8.02 ± 0.08 |
| `scan_delims_cl_sbcl` | 160.3 ± 0.9 | 158.4 | 161.9 | 8.49 ± 0.09 |
| `scan_delims_pypy` | 193.6 ± 0.4 | 192.7 | 194.3 | 10.26 ± 0.10 |
| `scan_delims_python` | 662.9 ± 3.5 | 659.1 | 671.2 | 35.11 ± 0.38 |
