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
- Occasionally some other langs.

Summary: in these _very limited_ microbenchmarks, the reg-CC implementation of Astil Forth trounces VM interpreters, approximates other JITs, and vaguely approaches Clang C with `-O2` (x1.5-x2).

Naming:
- `_aot`     -- Astil reg-CC as AOT-compiled executable.
- `_reg`     -- Astil reg-CC in JIT mode.
- `_asm_aot` -- Astil reg-CC in AOT mode.
- `_stack`   -- Astil stack-CC in JIT mode.

Each `_reg` and `_stack` benchmark includes the cost of bootstrapping the entire language first. Reg-CC takes longer to bootstrap because it has more features.

Notes:

- All measurements were done on M3 Pro.
- CPU microbenchmarks are sensitive to code layout and instruction selection. Small source changes can shift CPU frontend, cache, and branch-prediction behavior; on M3 Pro, we have seen cosmetic-looking changes move results by up to ≈10% or up to 10ms depending on benchmark runtime. Avoid over-generalizing differences in that range.
- Current suite records only wall time, not total CPU time. Some GC-based engines spend a lot of CPU/power on background threads.
- In many benchmarks, _startup time skews the measurement_. Adjust them by the "baseline" metrics when comparing.

## VERSIONS

```
clang 22.1.4

gforth 0.7.3

luajit 2.1.1753364724

bun 1.3.10

sbcl 2.6.4

zig 0.16.0

go version go1.25.1 darwin/arm64

openjdk version "25" 2025-09-16

[pypy 7.3.17 with gcc apple llvm 16.0.0 (clang-1600.0.26.4)]
python 3.10.14 (39dc8d3c85a7, nov 09 2024, 22:49:03)

python 3.14.4
```

## NONE

| Command | Mean [µs] | Min [µs] | Max [µs] | Relative |
|:---|---:|---:|---:|---:|
| `none_astil_stack` | 980.4 ± 54.1 | 849.1 | 1477.9 | 1.00 |
| `none_astil_reg` | 989.5 ± 53.9 | 845.2 | 1837.2 | 1.01 ± 0.08 |

## BASELINE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `baseline_luajit` | 2.6 ± 0.5 | 1.6 | 3.6 | 1.00 |
| `baseline_astil_stack` | 3.2 ± 0.1 | 3.0 | 3.5 | 1.20 ± 0.21 |
| `baseline_gforth` | 3.7 ± 0.4 | 2.6 | 4.6 | 1.42 ± 0.30 |
| `baseline_js_bun` | 3.9 ± 0.4 | 2.9 | 4.7 | 1.48 ± 0.30 |
| `baseline_astil_reg` | 12.7 ± 0.3 | 12.2 | 13.3 | 4.83 ± 0.84 |
| `baseline_cl_sbcl` | 12.9 ± 0.5 | 11.6 | 14.0 | 4.90 ± 0.87 |
| `baseline_pypy` | 15.6 ± 0.5 | 14.1 | 16.7 | 5.94 ± 1.04 |
| `baseline_python` | 16.6 ± 0.5 | 14.9 | 17.7 | 6.32 ± 1.11 |

## BUBBLE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `bubble_clang` | 18.4 ± 0.3 | 17.8 | 19.4 | 1.00 |
| `bubble_astil_aot` | 30.0 ± 0.4 | 29.1 | 30.9 | 1.63 ± 0.03 |
| `bubble_js_bun` | 36.1 ± 1.1 | 34.5 | 42.1 | 1.96 ± 0.07 |
| `bubble_luajit` | 37.3 ± 1.1 | 35.9 | 40.2 | 2.03 ± 0.07 |
| `bubble_astil_reg` | 41.9 ± 0.8 | 40.2 | 44.2 | 2.28 ± 0.06 |
| `bubble_cl_sbcl` | 44.2 ± 1.1 | 41.8 | 46.2 | 2.40 ± 0.07 |
| `bubble_pypy` | 87.3 ± 0.6 | 85.5 | 88.2 | 4.75 ± 0.09 |
| `bubble_astil_stack` | 97.9 ± 0.8 | 96.1 | 100.7 | 5.33 ± 0.10 |
| `bubble_gforth` | 213.5 ± 9.4 | 203.8 | 233.4 | 11.62 ± 0.55 |
| `bubble_python` | 1702.5 ± 24.5 | 1678.3 | 1755.5 | 92.64 ± 2.11 |

## PRIME SIEVE

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `sieve_clang` | 37.0 ± 0.2 | 36.6 | 37.5 | 1.00 |
| `sieve_cl_sbcl` | 56.7 ± 1.6 | 54.0 | 60.3 | 1.53 ± 0.04 |
| `sieve_astil_aot` | 57.7 ± 1.9 | 55.2 | 61.7 | 1.56 ± 0.05 |
| `sieve_js_bun` | 61.4 ± 1.7 | 58.9 | 64.9 | 1.66 ± 0.05 |
| `sieve_luajit` | 64.8 ± 0.5 | 63.6 | 66.0 | 1.75 ± 0.02 |
| `sieve_astil_reg` | 69.5 ± 1.9 | 66.4 | 72.0 | 1.88 ± 0.05 |
| `sieve_pypy` | 162.7 ± 0.3 | 162.1 | 163.2 | 4.40 ± 0.02 |
| `sieve_astil_stack` | 178.6 ± 1.5 | 177.1 | 182.8 | 4.83 ± 0.05 |
| `sieve_gforth` | 441.3 ± 5.9 | 433.5 | 454.0 | 11.93 ± 0.17 |
| `sieve_python` | 3866.1 ± 16.2 | 3839.1 | 3894.2 | 104.53 ± 0.70 |

## REVERSE STRING

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `reverse_string_clang` | 13.3 ± 1.0 | 12.7 | 15.8 | 1.00 |
| `reverse_string_astil_aot` | 30.0 ± 0.2 | 29.6 | 30.6 | 2.26 ± 0.16 |
| `reverse_string_js_bun` | 37.1 ± 0.4 | 35.9 | 38.4 | 2.79 ± 0.20 |
| `reverse_string_astil_reg` | 42.4 ± 0.3 | 41.9 | 43.1 | 3.18 ± 0.23 |
| `reverse_string_luajit` | 43.8 ± 0.8 | 42.1 | 47.2 | 3.29 ± 0.25 |
| `reverse_string_cl_sbcl` | 104.0 ± 2.4 | 100.6 | 110.4 | 7.82 ± 0.59 |
| `reverse_string_pypy` | 132.7 ± 1.3 | 130.3 | 135.1 | 9.98 ± 0.73 |
| `reverse_string_astil_stack` | 228.7 ± 0.4 | 227.8 | 229.1 | 17.19 ± 1.24 |
| `reverse_string_gforth` | 526.3 ± 1.0 | 524.6 | 527.5 | 39.56 ± 2.86 |
| `reverse_string_python` | 1686.0 ± 7.3 | 1676.3 | 1695.4 | 126.73 ± 9.17 |

## FIB_LOOP

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_astil_asm_aot` | 27.0 ± 0.8 | 26.2 | 28.5 | 1.00 |
| `fib_loop_clang` | 30.7 ± 0.7 | 29.7 | 32.7 | 1.14 ± 0.04 |
| `fib_loop_js_bun` | 37.1 ± 0.9 | 36.0 | 42.0 | 1.37 ± 0.05 |
| `fib_loop_astil_aot` | 51.0 ± 0.6 | 49.7 | 52.0 | 1.88 ± 0.06 |
| `fib_loop_cl_sbcl` | 60.8 ± 1.1 | 58.3 | 62.6 | 2.25 ± 0.08 |
| `fib_loop_astil_reg` | 61.5 ± 3.8 | 53.2 | 64.2 | 2.28 ± 0.16 |
| `fib_loop_luajit` | 95.4 ± 0.4 | 93.9 | 96.0 | 3.53 ± 0.10 |
| `fib_loop_pypy` | 215.3 ± 1.5 | 212.5 | 218.4 | 7.96 ± 0.23 |
| `fib_loop_astil_stack` | 286.8 ± 8.2 | 277.9 | 301.5 | 10.61 ± 0.43 |
| `fib_loop_gforth` | 597.7 ± 18.2 | 573.4 | 620.3 | 22.10 ± 0.92 |
| `fib_loop_python` | 3233.7 ± 11.0 | 3222.3 | 3257.9 | 119.57 ± 3.46 |

## FIB_LOOP_BIG

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_loop_big_clang` | 15.6 ± 0.4 | 15.0 | 16.8 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 17.8 ± 0.4 | 16.5 | 18.5 | 1.14 ± 0.04 |
| `fib_loop_big_astil_asm_reg` | 30.3 ± 0.3 | 29.5 | 30.9 | 1.94 ± 0.06 |
| `fib_loop_big_astil_aot` | 64.7 ± 0.2 | 64.3 | 65.4 | 4.14 ± 0.12 |
| `fib_loop_big_astil_reg` | 76.9 ± 0.2 | 76.5 | 77.3 | 4.92 ± 0.14 |
| `fib_loop_big_cl_sbcl` | 263.9 ± 0.9 | 262.2 | 265.4 | 16.87 ± 0.48 |
| `fib_loop_big_pypy` | 358.4 ± 1.0 | 357.3 | 359.7 | 22.91 ± 0.65 |
| `fib_loop_big_js_bun` | 421.7 ± 9.9 | 414.2 | 449.3 | 26.96 ± 0.99 |
| `fib_loop_big_python` | 1711.0 ± 7.2 | 1702.6 | 1728.3 | 109.36 ± 3.13 |

## FIB_RECURSIVE: fib(36)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fib_rec_clang` | 37.2 ± 0.6 | 35.9 | 38.9 | 1.00 |
| `fib_rec_astil_aot` | 57.1 ± 0.7 | 56.1 | 59.4 | 1.53 ± 0.03 |
| `fib_rec_luajit` | 68.6 ± 2.8 | 64.6 | 76.6 | 1.84 ± 0.08 |
| `fib_rec_js_bun` | 69.1 ± 1.0 | 67.5 | 71.8 | 1.86 ± 0.04 |
| `fib_rec_astil_reg` | 69.6 ± 0.5 | 68.8 | 71.3 | 1.87 ± 0.03 |
| `fib_rec_cl_sbcl` | 90.6 ± 0.8 | 88.9 | 91.9 | 2.43 ± 0.05 |
| `fib_rec_pypy` | 137.8 ± 1.9 | 135.6 | 143.5 | 3.70 ± 0.08 |
| `fib_rec_astil_stack` | 246.3 ± 5.2 | 240.5 | 255.4 | 6.62 ± 0.18 |
| `fib_rec_gforth` | 340.7 ± 3.3 | 337.3 | 348.3 | 9.15 ± 0.17 |
| `fib_rec_python` | 1564.1 ± 11.8 | 1546.7 | 1589.5 | 42.01 ± 0.75 |

## CONST FOLD

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `const_fold_folded_astil_aot` | 9.8 ± 0.3 | 9.2 | 10.8 | 1.00 |
| `const_fold_folded_astil_reg` | 21.9 ± 0.3 | 21.4 | 22.5 | 2.24 ± 0.07 |
| `const_fold_runtime_astil_aot` | 175.0 ± 5.1 | 170.4 | 182.5 | 17.85 ± 0.71 |
| `const_fold_runtime_astil_reg` | 182.9 ± 0.5 | 181.8 | 183.8 | 18.66 ± 0.51 |

## FNV-1A 64

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `fnv1a64_clang` | 34.6 ± 0.3 | 34.2 | 36.8 | 1.00 |
| `fnv1a64_astil_aot` | 35.7 ± 0.5 | 34.8 | 36.7 | 1.03 ± 0.02 |
| `fnv1a64_astil_reg` | 48.4 ± 0.5 | 47.3 | 49.5 | 1.40 ± 0.02 |
| `fnv1a64_cl_sbcl` | 49.2 ± 1.5 | 46.7 | 51.7 | 1.42 ± 0.04 |
| `fnv1a64_pypy` | 83.9 ± 0.4 | 83.0 | 84.7 | 2.43 ± 0.02 |
| `fnv1a64_gforth` | 135.5 ± 2.9 | 131.9 | 142.7 | 3.92 ± 0.09 |
| `fnv1a64_astil_stack` | 167.6 ± 9.5 | 156.9 | 184.1 | 4.84 ± 0.28 |
| `fnv1a64_luajit` | 278.5 ± 0.6 | 277.7 | 279.3 | 8.05 ± 0.07 |
| `fnv1a64_js_bun` | 418.5 ± 3.6 | 411.9 | 426.5 | 12.10 ± 0.14 |
| `fnv1a64_python` | 2467.9 ± 12.1 | 2454.8 | 2491.3 | 71.34 ± 0.67 |

## SCAN DELIMS

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `scan_delims_c_simd` | 18.9 ± 0.2 | 18.5 | 19.6 | 1.00 |
| `scan_delims_astil_simd_aot` | 19.1 ± 0.2 | 18.8 | 19.6 | 1.01 ± 0.01 |
| `scan_delims_astil_simd_reg` | 32.5 ± 0.5 | 32.0 | 36.2 | 1.72 ± 0.03 |
| `scan_delims_c_naive` | 50.9 ± 0.2 | 50.6 | 51.4 | 2.69 ± 0.02 |
| `scan_delims_luajit` | 86.8 ± 0.6 | 85.3 | 87.7 | 4.59 ± 0.05 |
| `scan_delims_js_bun` | 136.2 ± 0.5 | 135.3 | 137.1 | 7.20 ± 0.07 |
| `scan_delims_astil_naive_reg` | 148.7 ± 0.2 | 148.3 | 149.2 | 7.86 ± 0.07 |
| `scan_delims_cl_sbcl` | 160.0 ± 0.8 | 158.4 | 161.5 | 8.46 ± 0.08 |
| `scan_delims_pypy` | 193.7 ± 0.3 | 193.0 | 194.3 | 10.24 ± 0.09 |
| `scan_delims_python` | 662.2 ± 0.6 | 661.6 | 663.7 | 35.01 ± 0.30 |

## BINARY TREE

| Command | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `bin_tree_cl_sbcl` | 0.305 ± 0.001 | 0.304 | 0.307 | 1.00 |
| `bin_tree_java` | 0.311 ± 0.003 | 0.305 | 0.315 | 1.02 ± 0.01 |
| `bin_tree_astil_aot` | 0.313 ± 0.001 | 0.313 | 0.315 | 1.03 ± 0.00 |
| `bin_tree_astil_reg` | 0.327 ± 0.002 | 0.325 | 0.330 | 1.07 ± 0.01 |
| `bin_tree_js_bun_lucky` | 0.403 ± 0.009 | 0.390 | 0.416 | 1.32 ± 0.03 |
| `bin_tree_zig` | 0.418 ± 0.000 | 0.417 | 0.418 | 1.37 ± 0.00 |
| `bin_tree_clang` | 1.045 ± 0.009 | 1.030 | 1.057 | 3.42 ± 0.03 |
| `bin_tree_js_bun` | 1.061 ± 0.009 | 1.050 | 1.077 | 3.47 ± 0.03 |
| `bin_tree_go` | 1.123 ± 0.006 | 1.113 | 1.135 | 3.68 ± 0.02 |
| `bin_tree_luajit` | 2.029 ± 0.008 | 2.018 | 2.044 | 6.64 ± 0.03 |
| `bin_tree_astil_stack` | 2.281 ± 0.013 | 2.265 | 2.298 | 7.47 ± 0.05 |
| `bin_tree_pypy` | 2.642 ± 0.028 | 2.604 | 2.682 | 8.65 ± 0.09 |
| `bin_tree_gforth` | 4.784 ± 0.185 | 4.693 | 5.307 | 15.66 ± 0.61 |
| `bin_tree_python` | 8.198 ± 0.028 | 8.159 | 8.233 | 26.84 ± 0.12 |

## BINARY TREE BULK

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `bin_tree_clang_bulk` | 146.7 ± 2.3 | 143.8 | 151.1 | 1.00 |
| `bin_tree_astil_aot_bulk` | 194.9 ± 1.9 | 191.9 | 199.5 | 1.33 ± 0.02 |
| `bin_tree_astil_reg_bulk` | 206.0 ± 1.4 | 203.6 | 208.1 | 1.40 ± 0.02 |
| `bin_tree_go_bulk` | 376.9 ± 9.9 | 360.2 | 393.3 | 2.57 ± 0.08 |
| `bin_tree_astil_stack_bulk` | 1614.6 ± 28.9 | 1597.1 | 1675.6 | 11.01 ± 0.26 |
| `bin_tree_gforth_bulk` | 1633.9 ± 24.4 | 1593.4 | 1671.7 | 11.14 ± 0.24 |
