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

| Command | Wall [ms] | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `none_astil_stack` | 1.065 ± 0.052 | 742.0 ± 29.1 | 1.341 ± 0.016 | 1.00 |
| `none_astil_reg` | 1.16 ± 0.34 | 818.3 ± 202.0 | 1.409 ± 0.019 | 1.09 |

## BASELINE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 2.41 ± 0.48 | 0.778 ± 0.035 | 1.415 ± 0.014 | 1.00 |
| `baseline_astil_stack` | 3.161 ± 0.079 | 2.786 ± 0.067 | 1.754 ± 0.023 | 1.31 |
| `baseline_gforth` | 3.46 ± 0.36 | 1.777 ± 0.050 | 2.075 ± 0.023 | 1.44 |
| `baseline_js_bun` | 7.37 ± 0.44 | 5.44 ± 0.19 | 19.216 ± 0.075 | 3.06 |
| `baseline_cl_sbcl` | 12.14 ± 0.61 | 9.79 ± 0.33 | 39.199 ± 0.034 | 5.04 |
| `baseline_astil_reg` | 12.95 ± 0.14 | 12.53 ± 0.13 | 2.389 ± 0.022 | 5.37 |
| `baseline_pypy` | 15.16 ± 0.60 | 12.94 ± 0.28 | 28.036 ± 0.054 | 6.29 |
| `baseline_python` | 16.15 ± 0.47 | 13.46 ± 0.21 | 11.551 ± 0.085 | 6.70 |

> Warning: unstable wall time: `baseline_luajit` (middle 50% spans 36.6%); `baseline_gforth` (middle 50% spans 16.9%).

## BUBBLE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 300.0 ± 2.3 | 299.4 ± 2.3 | 1.353 ± 0.014 | 1.00 |
| `bubble_astil_aot` | 429.5 ± 4.1 | 428.7 ± 4.2 | 1.341 ± 0.017 | 1.43 |
| `bubble_js_bun` | 435.5 ± 1.1 | 436.7 ± 1.6 | 28.559 ± 0.097 | 1.45 |
| `bubble_astil_reg` | 448.2 ± 7.1 | 447.5 ± 7.0 | 2.578 ± 0.035 | 1.49 |
| `bubble_cl_sbcl` | 501.91 ± 0.99 | 498.47 ± 0.99 | 40.328 ± 0.035 | 1.67 |
| `bubble_luajit` | 580.1 ± 13.4 | 577.8 ± 13.3 | 2.0 ± 0.0 | 1.93 |
| `bubble_pypy` | 1144.1 ± 8.6 | 1140.4 ± 7.9 | 31.63 ± 0.49 | 3.81 |
| `bubble_astil_stack` | 1552.7 ± 14.3 | 1550.4 ± 11.6 | 2.013 ± 0.026 | 5.18 |
| `bubble_gforth` | 3757.2 ± 105.9 | 3754.6 ± 105.1 | 2.326 ± 0.026 | 12.52 |
| `bubble_python` | 26820.2 ± 82.7 | 26809.6 ± 84.1 | 12.969 ± 0.066 | 89.39 |

## PRIME SIEVE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 144.90 ± 0.35 | 144.29 ± 0.33 | 1.174 ± 0.017 | 1.00 |
| `sieve_cl_sbcl` | 183.9 ± 5.1 | 180.8 ± 4.8 | 40.170 ± 0.023 | 1.27 |
| `sieve_js_bun` | 225.2 ± 3.3 | 224.9 ± 4.0 | 27.347 ± 0.049 | 1.55 |
| `sieve_astil_aot` | 232.60 ± 0.28 | 231.93 ± 0.28 | 1.200 ± 0.017 | 1.61 |
| `sieve_astil_reg` | 245.0 ± 2.2 | 244.4 ± 2.1 | 2.384 ± 0.021 | 1.69 |
| `sieve_luajit` | 250.4 ± 1.8 | 247.9 ± 1.4 | 1.653 ± 0.017 | 1.73 |
| `sieve_pypy` | 586.29 ± 0.60 | 583.56 ± 0.98 | 31.64 ± 0.16 | 4.05 |
| `sieve_astil_stack` | 703.9 ± 2.7 | 703.2 ± 2.7 | 1.7 ± 0.0 | 4.86 |
| `sieve_gforth` | 1761.0 ± 35.4 | 1757.3 ± 33.4 | 2.106 ± 0.026 | 12.15 |
| `sieve_python` | 15397.0 ± 25.5 | 15389.2 ± 22.5 | 11.86 ± 0.25 | 106.26 |

## REVERSE STRING

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 96.66 ± 0.60 | 96.03 ± 0.47 | 1.2 ± 0.0 | 1.00 |
| `reverse_string_js_bun` | 221.4 ± 1.0 | 225.1 ± 1.2 | 30.822 ± 0.083 | 2.29 |
| `reverse_string_astil_aot` | 234.31 ± 0.90 | 233.66 ± 0.91 | 1.2 ± 0.0 | 2.42 |
| `reverse_string_astil_reg` | 248.5 ± 5.3 | 247.7 ± 5.1 | 2.381 ± 0.014 | 2.57 |
| `reverse_string_luajit` | 326.0 ± 1.4 | 324.04 ± 0.91 | 1.7 ± 0.0 | 3.37 |
| `reverse_string_cl_sbcl` | 734.5 ± 2.9 | 730.4 ± 2.8 | 39.925 ± 0.028 | 7.60 |
| `reverse_string_pypy` | 929.9 ± 15.1 | 925.5 ± 11.7 | 32.44 ± 0.39 | 9.62 |
| `reverse_string_astil_stack` | 1834.1 ± 18.4 | 1827.2 ± 11.7 | 1.738 ± 0.007 | 18.98 |
| `reverse_string_gforth` | 4218.8 ± 15.1 | 4214.8 ± 13.8 | 2.094 ± 0.031 | 43.65 |
| `reverse_string_python` | 13365.7 ± 23.7 | 13355.2 ± 22.1 | 11.64 ± 0.16 | 138.28 |

## FIB_LOOP

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_clang` | 101.2 ± 1.1 | 100.62 ± 0.97 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_astil_asm_aot` | 102.57 ± 0.44 | 102.02 ± 0.41 | 1.163 ± 0.013 | 1.01 |
| `fib_loop_cl_sbcl` | 194.6 ± 2.1 | 191.2 ± 2.1 | 92.365 ± 0.045 | 1.92 |
| `fib_loop_astil_aot` | 198.0 ± 4.0 | 197.4 ± 3.9 | 1.2 ± 0.0 | 1.96 |
| `fib_loop_astil_reg` | 209.5 ± 4.4 | 208.8 ± 4.3 | 2.384 ± 0.021 | 2.07 |
| `fib_loop_js_bun` | 274.2 ± 1.5 | 275.3 ± 1.6 | 27.83 ± 0.12 | 2.71 |
| `fib_loop_luajit` | 371.8 ± 1.9 | 370.0 ± 1.5 | 1.591 ± 0.017 | 3.67 |
| `fib_loop_pypy` | 811.5 ± 12.2 | 807.4 ± 8.9 | 31.36 ± 0.41 | 8.02 |
| `fib_loop_astil_stack` | 1129.3 ± 22.8 | 1127.1 ± 20.4 | 1.753 ± 0.026 | 11.15 |
| `fib_loop_gforth` | 2302.5 ± 49.2 | 2298.2 ± 50.8 | 2.103 ± 0.032 | 22.74 |
| `fib_loop_python` | 12936.7 ± 65.6 | 12926.4 ± 63.7 | 11.53 ± 0.17 | 127.79 |

## FIB_LOOP_BIG

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 116.1 ± 1.5 | 115.5 ± 1.4 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 130.5 ± 4.2 | 129.9 ± 4.2 | 1.2 ± 0.0 | 1.12 |
| `fib_loop_big_astil_asm_reg` | 146.7 ± 3.9 | 146.1 ± 4.0 | 2.4 ± 0.0 | 1.26 |
| `fib_loop_big_astil_aot` | 504.4 ± 7.4 | 503.5 ± 7.4 | 1.2 ± 0.0 | 4.34 |
| `fib_loop_big_astil_reg` | 516.6 ± 7.2 | 515.7 ± 7.2 | 2.4 ± 0.0 | 4.45 |
| `fib_loop_big_cl_sbcl` | 1988.0 ± 16.3 | 1985.3 ± 12.0 | 96.678 ± 0.053 | 17.12 |
| `fib_loop_big_pypy` | 2748.8 ± 7.3 | 2745.9 ± 7.7 | 35.972 ± 0.081 | 23.67 |
| `fib_loop_big_js_bun` | 3251.0 ± 10.5 | 3269.5 ± 4.8 | 60.966 ± 0.034 | 28.00 |
| `fib_loop_big_python` | 13551.7 ± 26.9 | 13541.3 ± 22.6 | 11.47 ± 0.16 | 116.70 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 153.8 ± 4.3 | 153.2 ± 4.4 | 1.145 ± 0.011 | 1.00 |
| `fib_rec_astil_aot` | 233.7 ± 4.1 | 233.0 ± 4.2 | 1.163 ± 0.014 | 1.52 |
| `fib_rec_astil_reg` | 242.9 ± 6.6 | 242.2 ± 6.5 | 2.406 ± 0.037 | 1.58 |
| `fib_rec_js_bun` | 261.0 ± 8.6 | 259.6 ± 7.4 | 26.108 ± 0.071 | 1.70 |
| `fib_rec_luajit` | 277.0 ± 10.5 | 275.0 ± 10.3 | 1.666 ± 0.013 | 1.80 |
| `fib_rec_cl_sbcl` | 333.0 ± 10.3 | 329.7 ± 10.3 | 38.998 ± 0.020 | 2.16 |
| `fib_rec_pypy` | 469.4 ± 3.0 | 467.1 ± 2.5 | 46.75 ± 0.13 | 3.05 |
| `fib_rec_astil_stack` | 998.2 ± 18.5 | 997.3 ± 18.6 | 1.8 ± 0.0 | 6.49 |
| `fib_rec_gforth` | 1430.5 ± 12.6 | 1427.3 ± 11.3 | 2.097 ± 0.039 | 9.30 |
| `fib_rec_python` | 6527.6 ± 18.1 | 6520.5 ± 15.5 | 11.58 ± 0.17 | 42.43 |

## CONST FOLD

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 0.140 ± 0.003 | 0.140 ± 0.003 | 1.160 ± 0.011 | 1.00 |
| `const_fold_folded_astil_reg` | 0.146 ± 0.001 | 0.146 ± 0.001 | 2.382 ± 0.018 | 1.04 |
| `const_fold_runtime_astil_aot` | 2.726 ± 0.012 | 2.724 ± 0.010 | 1.163 ± 0.014 | 19.42 |
| `const_fold_runtime_astil_reg` | 2.733 ± 0.003 | 2.732 ± 0.003 | 2.419 ± 0.034 | 19.48 |

## FNV-1A 64

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_astil_aot` | 134.61 ± 0.30 | 134.03 ± 0.28 | 1.176 ± 0.011 | 1.00 |
| `fnv1a64_clang` | 135.08 ± 0.46 | 134.47 ± 0.42 | 1.215 ± 0.016 | 1.00 |
| `fnv1a64_astil_reg` | 146.53 ± 0.32 | 145.93 ± 0.33 | 2.388 ± 0.023 | 1.09 |
| `fnv1a64_cl_sbcl` | 148.8 ± 1.1 | 146.1 ± 1.2 | 39.636 ± 0.030 | 1.11 |
| `fnv1a64_pypy` | 279.1 ± 1.3 | 276.5 ± 1.4 | 31.19 ± 0.12 | 2.07 |
| `fnv1a64_gforth` | 521.4 ± 2.3 | 519.3 ± 2.6 | 2.141 ± 0.037 | 3.87 |
| `fnv1a64_astil_stack` | 649.0 ± 43.7 | 647.8 ± 43.7 | 1.803 ± 0.016 | 4.82 |
| `fnv1a64_luajit` | 1109.4 ± 11.4 | 1106.5 ± 9.2 | 1.8 ± 0.0 | 8.24 |
| `fnv1a64_js_bun` | 1575.3 ± 42.9 | 1576.9 ± 42.6 | 29.082 ± 0.094 | 11.70 |
| `fnv1a64_python` | 9825.6 ± 32.8 | 9818.4 ± 28.8 | 11.60 ± 0.14 | 72.99 |

> Warning: unstable wall time: `fnv1a64_astil_stack` (middle 50% spans 10.1%).

## SCAN DELIMS

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 141.00 ± 0.77 | 140.41 ± 0.76 | 1.211 ± 0.014 | 1.00 |
| `scan_delims_astil_simd_aot` | 143.64 ± 0.58 | 143.03 ± 0.59 | 1.2 ± 0.0 | 1.02 |
| `scan_delims_astil_simd_reg` | 155.9 ± 1.1 | 155.3 ± 1.1 | 2.411 ± 0.025 | 1.11 |
| `scan_delims_c_naive` | 399.09 ± 0.43 | 398.30 ± 0.42 | 1.216 ± 0.017 | 2.83 |
| `scan_delims_luajit` | 675.7 ± 13.7 | 672.6 ± 11.6 | 1.741 ± 0.014 | 4.79 |
| `scan_delims_js_bun` | 942.5 ± 5.2 | 946.5 ± 5.2 | 30.691 ± 0.056 | 6.68 |
| `scan_delims_astil_naive_reg` | 1095.7 ± 2.1 | 1094.8 ± 2.0 | 2.416 ± 0.024 | 7.77 |
| `scan_delims_cl_sbcl` | 1144.4 ± 18.8 | 1141.4 ± 18.8 | 40.337 ± 0.021 | 8.12 |
| `scan_delims_pypy` | 1419.9 ± 2.8 | 1416.9 ± 2.2 | 31.359 ± 0.092 | 10.07 |
| `scan_delims_python` | 5059.2 ± 70.6 | 5053.1 ± 67.0 | 11.67 ± 0.12 | 35.88 |

## BINARY TREE

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_cl_sbcl` | 0.305 ± 0.008 | 0.301 ± 0.006 | 112.520 ± 0.031 | 1.00 |
| `bin_tree_java` | 0.310 ± 0.001 | 0.393 ± 0.005 | 929.6 ± 11.6 | 1.02 |
| `bin_tree_astil_aot` | 0.316 ± 0.010 | 0.315 ± 0.009 | 25.193 ± 0.013 | 1.04 |
| `bin_tree_astil_reg` | 0.321 ± 0.004 | 0.320 ± 0.004 | 26.4 ± 0.0 | 1.05 |
| `bin_tree_js_bun_lucky` | 0.403 ± 0.008 | 0.594 ± 0.037 | 191.7 ± 22.0 | 1.32 |
| `bin_tree_zig` | 0.418 ± 0.001 | 0.418 ± 0.001 | 25.925 ± 0.026 | 1.37 |
| `bin_tree_clang` | 1.033 ± 0.001 | 1.032 ± 0.001 | 27.150 ± 0.014 | 3.38 |
| `bin_tree_js_bun` | 1.055 ± 0.006 | 1.244 ± 0.016 | 176.3 ± 6.7 | 3.46 |
| `bin_tree_go` | 1.131 ± 0.004 | 4.791 ± 0.078 | 43.6 ± 1.7 | 3.70 |
| `bin_tree_luajit` | 2.027 ± 0.012 | 2.025 ± 0.012 | 265.5 ± 30.7 | 6.64 |
| `bin_tree_astil_stack` | 2.187 ± 0.025 | 2.185 ± 0.023 | 25.8 ± 0.0 | 7.16 |
| `bin_tree_pypy` | 2.641 ± 0.039 | 2.638 ± 0.038 | 189.7 ± 70.4 | 8.65 |
| `bin_tree_gforth` | 4.713 ± 0.022 | 4.708 ± 0.018 | 28.050 ± 0.017 | 15.44 |
| `bin_tree_python` | 8.204 ± 0.022 | 8.199 ± 0.019 | 83.86 ± 0.16 | 26.88 |

## BINARY TREE BULK

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 143.9 ± 4.5 | 143.1 ± 4.5 | 35.869 ± 0.020 | 1.00 |
| `bin_tree_astil_aot_bulk` | 196.9 ± 7.8 | 195.5 ± 5.9 | 25.205 ± 0.008 | 1.37 |
| `bin_tree_astil_reg_bulk` | 203.3 ± 2.5 | 202.7 ± 2.5 | 26.419 ± 0.026 | 1.41 |
| `bin_tree_go_bulk` | 370.4 ± 12.2 | 722.5 ± 8.1 | 55.5 ± 6.2 | 2.57 |
| `bin_tree_astil_stack_bulk` | 1610.1 ± 19.6 | 1605.0 ± 14.7 | 25.769 ± 0.026 | 11.19 |
| `bin_tree_gforth_bulk` | 1621.3 ± 16.6 | 1619.3 ± 16.3 | 36.784 ± 0.060 | 11.27 |
