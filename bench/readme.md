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
luv 1.52.1

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
| `none_astil_reg` | 1.065 ± 0.056 | 776.4 ± 33.7 | 1.423 ± 0.005 | 1.00 |
| `none_astil_stack` | 1.066 ± 0.048 | 761.4 ± 31.7 | 1.329 ± 0.003 | 1.00 |

## BASELINE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 1.94 ± 0.12 | 0.740 ± 0.027 | 1.416 ± 0.014 | 1.00 |
| `baseline_gforth` | 2.521 ± 0.038 | 1.714 ± 0.028 | 2.075 ± 0.024 | 1.30 |
| `baseline_astil_stack` | 3.092 ± 0.058 | 2.762 ± 0.048 | 1.744 ± 0.019 | 1.59 |
| `baseline_js_bun` | 6.69 ± 0.40 | 5.32 ± 0.17 | 19.202 ± 0.074 | 3.44 |
| `baseline_cl_sbcl` | 12.19 ± 0.19 | 10.23 ± 0.20 | 39.202 ± 0.042 | 6.27 |
| `baseline_astil_reg` | 12.83 ± 0.24 | 12.44 ± 0.24 | 2.360 ± 0.005 | 6.59 |
| `baseline_pypy` | 13.81 ± 0.33 | 12.63 ± 0.28 | 28.035 ± 0.040 | 7.10 |
| `baseline_python` | 14.92 ± 0.45 | 13.07 ± 0.27 | 11.557 ± 0.035 | 7.67 |
| `baseline_java` | 37.72 ± 0.98 | 40.30 ± 0.89 | 34.95 ± 0.14 | 19.39 |

## BUBBLE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 294.5 ± 6.9 | 294.0 ± 6.9 | 1.3 ± 0.0 | 1.00 |
| `bubble_js_bun` | 431.4 ± 3.8 | 432.8 ± 4.4 | 28.631 ± 0.072 | 1.46 |
| `bubble_astil_aot` | 433.78 ± 0.69 | 433.06 ± 0.80 | 1.363 ± 0.017 | 1.47 |
| `bubble_astil_reg` | 454.3 ± 16.3 | 453.6 ± 16.3 | 2.6 ± 0.0 | 1.54 |
| `bubble_cl_sbcl` | 499.0 ± 1.5 | 496.1 ± 1.3 | 40.337 ± 0.041 | 1.69 |
| `bubble_java` | 534.2 ± 2.1 | 537.5 ± 2.6 | 35.591 ± 0.073 | 1.81 |
| `bubble_luajit` | 580.4 ± 12.0 | 578.3 ± 11.7 | 1.988 ± 0.017 | 1.97 |
| `bubble_pypy` | 1134.2 ± 1.9 | 1131.7 ± 1.6 | 31.37 ± 0.12 | 3.85 |
| `bubble_astil_stack` | 1533.9 ± 9.4 | 1533.0 ± 9.4 | 1.994 ± 0.021 | 5.21 |
| `bubble_gforth` | 3821.0 ± 221.5 | 3819.4 ± 221.6 | 2.331 ± 0.040 | 12.97 |
| `bubble_python` | 26821.9 ± 236.8 | 26818.5 ± 235.0 | 12.961 ± 0.033 | 91.06 |

## PRIME SIEVE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 144.58 ± 0.50 | 144.07 ± 0.47 | 1.2 ± 0.0 | 1.00 |
| `sieve_cl_sbcl` | 185.7 ± 5.5 | 183.4 ± 5.4 | 40.2 ± 0.0 | 1.28 |
| `sieve_js_bun` | 212.4 ± 5.6 | 212.1 ± 5.5 | 27.348 ± 0.064 | 1.47 |
| `sieve_astil_aot` | 225.0 ± 5.3 | 224.2 ± 5.2 | 1.2 ± 0.0 | 1.56 |
| `sieve_astil_reg` | 235.8 ± 5.1 | 235.2 ± 5.1 | 2.4 ± 0.0 | 1.63 |
| `sieve_luajit` | 247.74 ± 0.54 | 246.19 ± 0.49 | 1.6 ± 0.0 | 1.71 |
| `sieve_java` | 251.00 ± 0.65 | 259.33 ± 0.86 | 36.100 ± 0.098 | 1.74 |
| `sieve_pypy` | 582.0 ± 1.0 | 580.4 ± 1.0 | 31.672 ± 0.066 | 4.03 |
| `sieve_astil_stack` | 707.1 ± 3.7 | 706.5 ± 3.7 | 1.7 ± 0.0 | 4.89 |
| `sieve_gforth` | 1736.9 ± 22.7 | 1735.4 ± 22.6 | 2.094 ± 0.027 | 12.01 |
| `sieve_python` | 15283.9 ± 28.3 | 15281.4 ± 28.3 | 11.677 ± 0.063 | 105.71 |

## REVERSE STRING

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 95.98 ± 0.55 | 95.49 ± 0.50 | 1.2 ± 0.0 | 1.00 |
| `reverse_string_java` | 134.67 ± 0.81 | 141.77 ± 0.65 | 36.25 ± 0.21 | 1.40 |
| `reverse_string_js_bun` | 220.6 ± 1.2 | 224.1 ± 1.2 | 30.847 ± 0.057 | 2.30 |
| `reverse_string_astil_aot` | 232.1 ± 1.1 | 231.5 ± 1.1 | 1.2 ± 0.0 | 2.42 |
| `reverse_string_astil_reg` | 245.1 ± 2.2 | 244.5 ± 2.2 | 2.4 ± 0.0 | 2.55 |
| `reverse_string_luajit` | 322.24 ± 0.38 | 320.58 ± 0.28 | 1.7 ± 0.0 | 3.36 |
| `reverse_string_cl_sbcl` | 687.63 ± 0.95 | 685.03 ± 0.99 | 39.962 ± 0.021 | 7.16 |
| `reverse_string_pypy` | 917.5 ± 7.5 | 915.7 ± 7.5 | 32.37 ± 0.20 | 9.56 |
| `reverse_string_astil_stack` | 1803.4 ± 10.5 | 1802.6 ± 10.5 | 1.7 ± 0.0 | 18.79 |
| `reverse_string_gforth` | 4172.8 ± 3.9 | 4171.4 ± 3.8 | 2.081 ± 0.026 | 43.47 |
| `reverse_string_python` | 13249.4 ± 20.5 | 13247.0 ± 20.5 | 11.703 ± 0.026 | 138.04 |

## FIB_LOOP

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_clang` | 100.46 ± 0.69 | 99.99 ± 0.64 | 1.153 ± 0.016 | 1.00 |
| `fib_loop_astil_asm_aot` | 102.37 ± 0.72 | 101.90 ± 0.72 | 1.188 ± 0.016 | 1.02 |
| `fib_loop_java` | 140.9 ± 1.5 | 145.4 ± 1.4 | 35.471 ± 0.072 | 1.40 |
| `fib_loop_astil_aot` | 158.2 ± 1.3 | 157.5 ± 1.1 | 1.2 ± 0.0 | 1.57 |
| `fib_loop_astil_reg` | 193.0 ± 16.0 | 192.5 ± 16.0 | 2.369 ± 0.020 | 1.92 |
| `fib_loop_cl_sbcl` | 193.7 ± 1.7 | 191.1 ± 1.7 | 92.372 ± 0.055 | 1.93 |
| `fib_loop_js_bun` | 273.75 ± 0.56 | 275.17 ± 0.47 | 27.938 ± 0.043 | 2.73 |
| `fib_loop_luajit` | 369.84 ± 0.49 | 368.24 ± 0.35 | 1.591 ± 0.017 | 3.68 |
| `fib_loop_pypy` | 789.9 ± 54.9 | 788.0 ± 55.0 | 31.23 ± 0.42 | 7.86 |
| `fib_loop_astil_stack` | 1107.8 ± 8.6 | 1107.0 ± 8.6 | 1.744 ± 0.021 | 11.03 |
| `fib_loop_gforth` | 2304.7 ± 37.8 | 2303.2 ± 37.8 | 2.072 ± 0.024 | 22.94 |
| `fib_loop_python` | 12829.2 ± 55.7 | 12826.7 ± 55.7 | 11.57 ± 0.13 | 127.71 |

> Warning: unstable wall time: `fib_loop_astil_reg` (middle 50% spans 16.3%).

## FIB_LOOP_BIG

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 0.118 ± 0.004 | 0.118 ± 0.004 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 0.137 ± 0.001 | 0.136 ± 2.5e-04 | 1.2 ± 0.0 | 1.16 |
| `fib_loop_big_astil_asm_reg` | 0.149 ± 0.001 | 0.148 ± 0.001 | 2.4 ± 0.0 | 1.26 |
| `fib_loop_big_astil_aot` | 0.501 ± 0.006 | 0.500 ± 0.006 | 1.2 ± 0.0 | 4.25 |
| `fib_loop_big_astil_reg` | 0.516 ± 0.006 | 0.516 ± 0.006 | 2.4 ± 0.0 | 4.37 |
| `fib_loop_big_cl_sbcl` | 1.975 ± 0.004 | 1.974 ± 0.004 | 96.706 ± 0.047 | 16.74 |
| `fib_loop_big_java` | 2.495 ± 0.009 | 2.569 ± 0.006 | 456.2 ± 65.9 | 21.14 |
| `fib_loop_big_pypy` | 2.753 ± 0.022 | 2.751 ± 0.022 | 35.856 ± 0.093 | 23.33 |
| `fib_loop_big_js_bun` | 3.253 ± 0.018 | 3.271 ± 0.019 | 60.888 ± 0.056 | 27.56 |
| `fib_loop_big_python` | 13.485 ± 0.057 | 13.483 ± 0.057 | 11.652 ± 0.030 | 114.27 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 154.1 ± 5.5 | 153.6 ± 5.5 | 1.148 ± 0.014 | 1.00 |
| `fib_rec_java` | 196.4 ± 1.8 | 199.5 ± 1.9 | 35.28 ± 0.16 | 1.27 |
| `fib_rec_astil_aot` | 233.0 ± 2.5 | 232.3 ± 2.8 | 1.191 ± 0.017 | 1.51 |
| `fib_rec_astil_reg` | 244.4 ± 3.5 | 243.8 ± 3.5 | 2.378 ± 0.026 | 1.59 |
| `fib_rec_js_bun` | 258.3 ± 3.4 | 257.2 ± 3.2 | 26.053 ± 0.089 | 1.68 |
| `fib_rec_luajit` | 277.1 ± 12.6 | 275.5 ± 12.6 | 1.661 ± 0.011 | 1.80 |
| `fib_rec_cl_sbcl` | 334.1 ± 8.0 | 331.5 ± 8.0 | 39.003 ± 0.025 | 2.17 |
| `fib_rec_pypy` | 463.8 ± 2.7 | 462.1 ± 2.6 | 46.68 ± 0.14 | 3.01 |
| `fib_rec_astil_stack` | 1005.3 ± 24.4 | 1004.7 ± 24.4 | 1.758 ± 0.019 | 6.52 |
| `fib_rec_gforth` | 1421.4 ± 16.5 | 1419.9 ± 16.4 | 2.075 ± 0.032 | 9.22 |
| `fib_rec_python` | 6494.7 ± 33.3 | 6492.2 ± 33.3 | 11.666 ± 0.097 | 42.14 |

## CONST FOLD

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 0.142 ± 0.004 | 0.141 ± 0.004 | 1.2 ± 0.0 | 1.00 |
| `const_fold_folded_astil_reg` | 0.154 ± 0.004 | 0.154 ± 0.004 | 2.393 ± 0.047 | 1.09 |
| `const_fold_runtime_astil_aot` | 2.710 ± 0.002 | 2.709 ± 0.002 | 1.2 ± 0.0 | 19.13 |
| `const_fold_runtime_astil_reg` | 2.717 ± 0.009 | 2.716 ± 0.009 | 2.4 ± 0.0 | 19.18 |

## FNV-1A 64

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_clang` | 135.09 ± 0.78 | 134.55 ± 0.71 | 1.2 ± 0.0 | 1.00 |
| `fnv1a64_astil_aot` | 139.6 ± 4.8 | 139.0 ± 4.7 | 1.2 ± 0.0 | 1.03 |
| `fnv1a64_cl_sbcl` | 149.8 ± 3.6 | 147.6 ± 3.6 | 39.7 ± 0.0 | 1.11 |
| `fnv1a64_astil_reg` | 150.0 ± 5.1 | 149.5 ± 5.1 | 2.4 ± 0.0 | 1.11 |
| `fnv1a64_java` | 173.68 ± 0.98 | 180.56 ± 0.89 | 35.97 ± 0.16 | 1.29 |
| `fnv1a64_pypy` | 277.1 ± 1.3 | 275.3 ± 1.2 | 31.225 ± 0.096 | 2.05 |
| `fnv1a64_gforth` | 534.1 ± 8.7 | 532.9 ± 8.9 | 2.125 ± 0.011 | 3.95 |
| `fnv1a64_astil_stack` | 643.0 ± 51.6 | 642.3 ± 51.6 | 1.808 ± 0.020 | 4.76 |
| `fnv1a64_luajit` | 1107.6 ± 6.6 | 1105.9 ± 6.7 | 1.8 ± 0.0 | 8.20 |
| `fnv1a64_js_bun` | 1545.0 ± 17.6 | 1546.8 ± 17.9 | 29.087 ± 0.089 | 11.44 |
| `fnv1a64_python` | 9744.3 ± 19.7 | 9741.7 ± 19.6 | 11.778 ± 0.046 | 72.13 |

## SCAN DELIMS

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 143.54 ± 0.38 | 143.02 ± 0.34 | 1.2 ± 0.0 | 1.00 |
| `scan_delims_astil_simd_aot` | 144.50 ± 0.80 | 143.84 ± 0.41 | 1.2 ± 0.0 | 1.01 |
| `scan_delims_astil_simd_reg` | 156.90 ± 0.24 | 156.37 ± 0.28 | 2.417 ± 0.071 | 1.09 |
| `scan_delims_java_simd` | 305.0 ± 3.3 | 403.0 ± 3.9 | 178.90 ± 0.71 | 2.13 |
| `scan_delims_c_naive` | 397.66 ± 0.60 | 396.82 ± 0.28 | 1.2 ± 0.0 | 2.77 |
| `scan_delims_luajit` | 666.22 ± 0.34 | 664.55 ± 0.36 | 1.741 ± 0.014 | 4.64 |
| `scan_delims_java` | 670.35 ± 0.83 | 678.43 ± 0.59 | 36.08 ± 0.11 | 4.67 |
| `scan_delims_js_bun` | 952.4 ± 1.6 | 956.0 ± 1.3 | 30.637 ± 0.096 | 6.64 |
| `scan_delims_astil_naive_reg` | 1091.2 ± 1.5 | 1090.5 ± 1.5 | 2.397 ± 0.014 | 7.60 |
| `scan_delims_cl_sbcl` | 1162.0 ± 9.9 | 1159.3 ± 10.0 | 40.350 ± 0.024 | 8.10 |
| `scan_delims_pypy` | 1413.04 ± 0.46 | 1410.88 ± 0.42 | 31.25 ± 0.19 | 9.84 |
| `scan_delims_python` | 5105.7 ± 75.2 | 5103.3 ± 75.2 | 11.700 ± 0.045 | 35.57 |

## BINARY TREE

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_java` | 0.296 ± 0.004 | 0.371 ± 0.017 | 911.0 ± 38.6 | 1.00 |
| `bin_tree_cl_sbcl` | 0.301 ± 3.7e-04 | 0.298 ± 3.3e-04 | 112.481 ± 0.039 | 1.02 |
| `bin_tree_astil_aot` | 0.319 ± 0.012 | 0.318 ± 0.012 | 25.207 ± 0.011 | 1.08 |
| `bin_tree_astil_reg` | 0.328 ± 0.005 | 0.327 ± 0.005 | 26.400 ± 0.021 | 1.11 |
| `bin_tree_js_bun_lucky` | 0.398 ± 0.008 | 0.582 ± 0.028 | 198.0 ± 14.9 | 1.34 |
| `bin_tree_zig` | 0.416 ± 2.7e-04 | 0.416 ± 2.4e-04 | 25.934 ± 0.026 | 1.41 |
| `bin_tree_clang` | 1.047 ± 0.012 | 1.047 ± 0.012 | 27.131 ± 0.014 | 3.54 |
| `bin_tree_js_bun` | 1.050 ± 0.007 | 1.244 ± 0.015 | 180.4 ± 20.2 | 3.55 |
| `bin_tree_go` | 1.124 ± 0.004 | 4.865 ± 0.026 | 44.55 ± 0.31 | 3.80 |
| `bin_tree_luajit` | 2.021 ± 0.009 | 2.019 ± 0.009 | 277.4 ± 13.7 | 6.83 |
| `bin_tree_astil_stack` | 2.204 ± 0.005 | 2.203 ± 0.005 | 25.778 ± 0.026 | 7.45 |
| `bin_tree_pypy` | 2.604 ± 0.021 | 2.602 ± 0.021 | 142.13 ± 0.22 | 8.80 |
| `bin_tree_gforth` | 4.722 ± 0.012 | 4.720 ± 0.012 | 28.069 ± 0.014 | 15.96 |
| `bin_tree_python` | 8.166 ± 0.048 | 8.164 ± 0.048 | 84.084 ± 0.030 | 27.60 |

## BINARY TREE BULK

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 146.6 ± 2.2 | 145.9 ± 2.2 | 35.873 ± 0.023 | 1.00 |
| `bin_tree_astil_aot_bulk` | 196.8 ± 2.9 | 195.9 ± 2.5 | 25.224 ± 0.013 | 1.34 |
| `bin_tree_astil_reg_bulk` | 207.40 ± 0.78 | 206.84 ± 0.77 | 26.400 ± 0.021 | 1.41 |
| `bin_tree_go_bulk` | 374.9 ± 12.6 | 719.0 ± 9.3 | 58.8 ± 8.1 | 2.56 |
| `bin_tree_gforth_bulk` | 1615.6 ± 12.5 | 1614.2 ± 12.5 | 36.750 ± 0.037 | 11.02 |
| `bin_tree_astil_stack_bulk` | 1620.3 ± 47.6 | 1619.6 ± 47.5 | 25.764 ± 0.023 | 11.05 |

## TCP CONNECTIONS

Measures 4096 concurrent connections with basic send + receive.

Wall time includes Python TCP driver work. After every connection closes, the driver kills and reaps the idle server; this work is also included in wall time. CPU time and peak mem/RSS measure only the server subprocess. Measurements use at most 5 runs.

| Command | Wall [ms] | CPU [ms] ↓ | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `tcp_conn_luajit_luv_coro` | 336.9 ± 8.8 | 65.2 ± 3.8 | 9.422 ± 0.072 | 1.00 |
| `tcp_conn_luajit_luv_evented` | 339.6 ± 11.3 | 65.7 ± 5.2 | 5.82 ± 0.14 | 1.01 |
| `tcp_conn_js_bun_evented` | 344.8 ± 15.9 | 78.3 ± 7.5 | 26.98 ± 0.12 | 1.20 |
| `tcp_conn_zig_pthread` | 469.0 ± 84.4 | 139.1 ± 18.2 | 65.878 ± 0.060 | 2.13 |
| `tcp_conn_astil_aot_pthread` | 610.7 ± 126.2 | 147.3 ± 33.1 | 65.80 ± 0.14 | 2.26 |
| `tcp_conn_astil_reg_pthread` | 392.8 ± 119.8 | 149.2 ± 7.1 | 67.25 ± 0.11 | 2.29 |
| `tcp_conn_clang_pthread` | 408.5 ± 176.0 | 162.7 ± 37.8 | 65.181 ± 0.018 | 2.50 |
| `tcp_conn_go_goroutine` | 371.0 ± 49.5 | 214.1 ± 11.5 | 18.41 ± 0.11 | 3.28 |
| `tcp_conn_python_asyncio` | 360.2 ± 21.3 | 239.1 ± 6.6 | 43.32 ± 0.28 | 3.67 |
| `tcp_conn_java_vthread` | 394.4 ± 13.6 | 495.1 ± 11.5 | 72.33 ± 0.93 | 7.59 |
| `tcp_conn_pypy_asyncio` | 830.5 ± 47.6 | 530.7 ± 14.2 | 88.6 ± 1.6 | 8.14 |
| `tcp_conn_java_thread` | 448.3 ± 121.4 | 705.8 ± 68.2 | 452.1 ± 47.3 | 10.82 |
| `tcp_conn_cl_sbcl_thread` | 567.4 ± 43.2 | 791.2 ± 179.4 | 725.5 ± 32.5 | 12.13 |
| `tcp_conn_gforth_task` | 3429.1 ± 77.5 | 3350.5 ± 71.7 | 68.694 ± 0.046 | 51.38 |
