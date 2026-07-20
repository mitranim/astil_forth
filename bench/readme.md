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
| `none_astil_stack` | 1.074 ± 0.061 | 737.6 ± 31.6 | 1.329 ± 0.005 | 1.00 |
| `none_astil_reg` | 1.115 ± 0.063 | 779.1 ± 34.5 | 1.407 ± 0.004 | 1.04 |

## BASELINE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 2.43 ± 0.48 | 0.775 ± 0.033 | 1.416 ± 0.014 | 1.00 |
| `baseline_astil_stack` | 3.160 ± 0.070 | 2.774 ± 0.059 | 1.7 ± 0.0 | 1.30 |
| `baseline_gforth` | 3.55 ± 0.38 | 1.773 ± 0.052 | 2.062 ± 0.010 | 1.46 |
| `baseline_js_bun` | 7.33 ± 0.39 | 5.46 ± 0.14 | 19.204 ± 0.069 | 3.01 |
| `baseline_cl_sbcl` | 12.27 ± 0.78 | 9.83 ± 0.52 | 39.209 ± 0.055 | 5.04 |
| `baseline_astil_reg` | 12.97 ± 0.12 | 12.53 ± 0.11 | 2.4 ± 0.0 | 5.33 |
| `baseline_pypy` | 15.41 ± 0.45 | 13.10 ± 0.23 | 28.030 ± 0.084 | 6.33 |
| `baseline_python` | 16.06 ± 0.41 | 13.57 ± 0.26 | 11.545 ± 0.099 | 6.60 |
| `baseline_java` | 36.47 ± 0.60 | 38.9 ± 1.1 | 34.78 ± 0.84 | 14.98 |

## BUBBLE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 300.6 ± 1.4 | 300.0 ± 1.4 | 1.334 ± 0.014 | 1.00 |
| `bubble_astil_aot` | 428.1 ± 3.0 | 427.3 ± 2.9 | 1.3 ± 0.0 | 1.42 |
| `bubble_js_bun` | 437.03 ± 0.82 | 439.07 ± 0.88 | 28.616 ± 0.042 | 1.45 |
| `bubble_astil_reg` | 451.5 ± 16.9 | 449.6 ± 14.7 | 2.600 ± 0.040 | 1.50 |
| `bubble_cl_sbcl` | 501.2 ± 1.1 | 498.26 ± 0.97 | 40.353 ± 0.028 | 1.67 |
| `bubble_java` | 536.9 ± 1.2 | 540.32 ± 0.91 | 35.756 ± 0.074 | 1.79 |
| `bubble_luajit` | 578.9 ± 12.4 | 576.8 ± 12.6 | 1.975 ± 0.014 | 1.93 |
| `bubble_pypy` | 1139.83 ± 0.97 | 1136.7 ± 1.3 | 31.372 ± 0.097 | 3.79 |
| `bubble_astil_stack` | 1558.9 ± 18.9 | 1556.8 ± 16.7 | 2.0 ± 0.0 | 5.19 |
| `bubble_gforth` | 3710.9 ± 129.7 | 3707.5 ± 130.2 | 2.345 ± 0.037 | 12.35 |
| `bubble_python` | 26796.6 ± 139.0 | 26783.6 ± 126.3 | 13.0 ± 0.0 | 89.16 |

## PRIME SIEVE

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 144.88 ± 0.68 | 144.29 ± 0.63 | 1.161 ± 0.012 | 1.00 |
| `sieve_cl_sbcl` | 183.7 ± 2.2 | 180.8 ± 1.8 | 40.177 ± 0.021 | 1.27 |
| `sieve_js_bun` | 221.0 ± 3.7 | 220.2 ± 2.9 | 27.391 ± 0.088 | 1.53 |
| `sieve_astil_aot` | 232.7 ± 8.5 | 231.6 ± 6.8 | 1.192 ± 0.011 | 1.61 |
| `sieve_astil_reg` | 243.7 ± 1.2 | 243.0 ± 1.3 | 2.400 ± 0.024 | 1.68 |
| `sieve_luajit` | 249.22 ± 0.27 | 247.19 ± 0.48 | 1.6 ± 0.0 | 1.72 |
| `sieve_java` | 252.67 ± 0.61 | 260.82 ± 0.90 | 36.219 ± 0.097 | 1.74 |
| `sieve_pypy` | 584.85 ± 0.71 | 582.22 ± 0.60 | 31.64 ± 0.16 | 4.04 |
| `sieve_astil_stack` | 704.5 ± 2.2 | 703.6 ± 2.2 | 1.753 ± 0.026 | 4.86 |
| `sieve_gforth` | 1762.2 ± 17.9 | 1759.7 ± 17.8 | 2.094 ± 0.022 | 12.16 |
| `sieve_python` | 15403.5 ± 12.3 | 15392.2 ± 12.4 | 12.06 ± 0.10 | 106.32 |

## REVERSE STRING

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 97.5 ± 1.6 | 96.9 ± 1.5 | 1.176 ± 0.016 | 1.00 |
| `reverse_string_java` | 136.50 ± 0.71 | 143.58 ± 0.65 | 36.22 ± 0.16 | 1.40 |
| `reverse_string_js_bun` | 221.8 ± 1.1 | 225.1 ± 1.1 | 30.900 ± 0.032 | 2.27 |
| `reverse_string_astil_aot` | 237.3 ± 7.6 | 236.0 ± 5.7 | 1.198 ± 0.015 | 2.43 |
| `reverse_string_astil_reg` | 247.2 ± 1.9 | 246.5 ± 1.9 | 2.4 ± 0.0 | 2.53 |
| `reverse_string_luajit` | 330.7 ± 12.5 | 328.4 ± 11.4 | 1.664 ± 0.014 | 3.39 |
| `reverse_string_cl_sbcl` | 709.1 ± 9.4 | 705.6 ± 8.9 | 39.966 ± 0.026 | 7.27 |
| `reverse_string_pypy` | 914.1 ± 3.8 | 911.6 ± 3.4 | 32.153 ± 0.084 | 9.37 |
| `reverse_string_astil_stack` | 1824.1 ± 18.5 | 1820.5 ± 14.8 | 1.744 ± 0.021 | 18.71 |
| `reverse_string_gforth` | 4196.3 ± 18.3 | 4192.0 ± 15.3 | 2.128 ± 0.034 | 43.03 |
| `reverse_string_python` | 13322.6 ± 10.5 | 13311.7 ± 9.9 | 11.750 ± 0.018 | 136.62 |

## FIB_LOOP

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_astil_asm_aot` | 102.6 ± 1.0 | 101.98 ± 0.84 | 1.169 ± 0.016 | 1.00 |
| `fib_loop_clang` | 102.7 ± 6.1 | 101.7 ± 4.2 | 1.151 ± 0.015 | 1.00 |
| `fib_loop_java` | 143.0 ± 1.5 | 147.1 ± 1.3 | 35.45 ± 0.11 | 1.39 |
| `fib_loop_cl_sbcl` | 191.84 ± 0.96 | 188.40 ± 0.75 | 92.318 ± 0.034 | 1.87 |
| `fib_loop_astil_aot` | 195.8 ± 5.8 | 195.2 ± 5.8 | 1.170 ± 0.016 | 1.91 |
| `fib_loop_astil_reg` | 207.5 ± 6.2 | 206.9 ± 6.2 | 2.385 ± 0.021 | 2.02 |
| `fib_loop_js_bun` | 275.1 ± 4.4 | 275.8 ± 4.6 | 27.81 ± 0.15 | 2.68 |
| `fib_loop_luajit` | 370.5 ± 1.6 | 368.6 ± 1.4 | 1.591 ± 0.017 | 3.61 |
| `fib_loop_pypy` | 773.5 ± 89.7 | 769.8 ± 88.9 | 31.39 ± 0.53 | 7.54 |
| `fib_loop_astil_stack` | 1150.9 ± 41.6 | 1144.0 ± 34.3 | 1.756 ± 0.024 | 11.22 |
| `fib_loop_gforth` | 2277.8 ± 62.0 | 2271.9 ± 58.7 | 2.104 ± 0.037 | 22.20 |
| `fib_loop_python` | 12959.7 ± 131.3 | 12935.8 ± 118.9 | 11.68 ± 0.24 | 126.29 |

## FIB_LOOP_BIG

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 0.119 ± 0.003 | 0.119 ± 0.003 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 0.133 ± 0.005 | 0.133 ± 0.005 | 1.2 ± 0.0 | 1.12 |
| `fib_loop_big_astil_asm_reg` | 0.145 ± 0.004 | 0.145 ± 0.004 | 2.4 ± 0.0 | 1.22 |
| `fib_loop_big_astil_aot` | 0.502 ± 0.008 | 0.502 ± 0.008 | 1.2 ± 0.0 | 4.22 |
| `fib_loop_big_astil_reg` | 0.516 ± 0.006 | 0.515 ± 0.006 | 2.381 ± 0.014 | 4.33 |
| `fib_loop_big_cl_sbcl` | 1.997 ± 0.016 | 1.996 ± 0.013 | 96.672 ± 0.043 | 16.76 |
| `fib_loop_big_java` | 2.615 ± 0.016 | 2.695 ± 0.015 | 504.59 ± 0.55 | 21.94 |
| `fib_loop_big_pypy` | 2.752 ± 0.005 | 2.748 ± 0.005 | 36.025 ± 0.088 | 23.09 |
| `fib_loop_big_js_bun` | 3.281 ± 0.058 | 3.300 ± 0.058 | 60.959 ± 0.070 | 27.53 |
| `fib_loop_big_python` | 13.613 ± 0.061 | 13.602 ± 0.064 | 11.61 ± 0.17 | 114.22 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 149.41 ± 0.84 | 148.80 ± 0.74 | 1.154 ± 0.017 | 1.00 |
| `fib_rec_java` | 198.3 ± 4.0 | 201.3 ± 4.5 | 35.362 ± 0.099 | 1.33 |
| `fib_rec_astil_aot` | 226.7 ± 1.8 | 226.1 ± 1.9 | 1.169 ± 0.017 | 1.52 |
| `fib_rec_astil_reg` | 239.8 ± 3.4 | 239.1 ± 3.5 | 2.394 ± 0.026 | 1.60 |
| `fib_rec_js_bun` | 259.7 ± 4.3 | 258.5 ± 4.3 | 26.125 ± 0.076 | 1.74 |
| `fib_rec_luajit` | 272.5 ± 6.4 | 270.4 ± 6.2 | 1.672 ± 0.020 | 1.82 |
| `fib_rec_cl_sbcl` | 323.1 ± 3.7 | 319.7 ± 2.7 | 39.006 ± 0.024 | 2.16 |
| `fib_rec_pypy` | 468.7 ± 2.8 | 465.9 ± 2.8 | 46.72 ± 0.13 | 3.14 |
| `fib_rec_astil_stack` | 1045.7 ± 68.3 | 1044.5 ± 67.7 | 1.762 ± 0.021 | 7.00 |
| `fib_rec_gforth` | 1421.7 ± 13.5 | 1419.8 ± 13.5 | 2.087 ± 0.024 | 9.52 |
| `fib_rec_python` | 6504.8 ± 20.1 | 6500.3 ± 18.0 | 11.56 ± 0.18 | 43.54 |

## CONST FOLD

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 0.134 ± 1.3e-04 | 0.134 ± 1.0e-04 | 1.168 ± 0.016 | 1.00 |
| `const_fold_folded_astil_reg` | 0.149 ± 0.004 | 0.149 ± 0.004 | 2.379 ± 0.011 | 1.11 |
| `const_fold_runtime_astil_aot` | 2.718 ± 0.011 | 2.717 ± 0.010 | 1.169 ± 0.017 | 20.22 |
| `const_fold_runtime_astil_reg` | 2.738 ± 0.020 | 2.734 ± 0.015 | 2.400 ± 0.014 | 20.36 |

## FNV-1A 64

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_clang` | 134.44 ± 0.46 | 133.83 ± 0.34 | 1.2 ± 0.0 | 1.00 |
| `fnv1a64_astil_aot` | 137.0 ± 3.7 | 136.4 ± 3.6 | 1.2 ± 0.0 | 1.02 |
| `fnv1a64_astil_reg` | 148.3 ± 4.0 | 147.8 ± 4.0 | 2.379 ± 0.011 | 1.10 |
| `fnv1a64_cl_sbcl` | 148.4 ± 1.3 | 145.8 ± 1.2 | 39.654 ± 0.032 | 1.10 |
| `fnv1a64_java` | 173.7 ± 1.2 | 180.1 ± 1.0 | 35.93 ± 0.12 | 1.29 |
| `fnv1a64_pypy` | 277.70 ± 0.36 | 274.81 ± 0.46 | 31.20 ± 0.12 | 2.07 |
| `fnv1a64_gforth` | 529.9 ± 8.5 | 527.8 ± 8.6 | 2.144 ± 0.020 | 3.94 |
| `fnv1a64_astil_stack` | 642.6 ± 43.1 | 641.3 ± 43.3 | 1.807 ± 0.020 | 4.78 |
| `fnv1a64_luajit` | 1112.8 ± 21.2 | 1106.5 ± 11.4 | 1.803 ± 0.014 | 8.28 |
| `fnv1a64_js_bun` | 1594.0 ± 31.9 | 1596.2 ± 31.7 | 29.141 ± 0.029 | 11.86 |
| `fnv1a64_python` | 9872.9 ± 75.5 | 9863.5 ± 69.9 | 11.70 ± 0.18 | 73.44 |

## SCAN DELIMS

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 141.61 ± 0.43 | 141.04 ± 0.45 | 1.2 ± 0.0 | 1.00 |
| `scan_delims_astil_simd_aot` | 143.36 ± 0.55 | 142.77 ± 0.58 | 1.2 ± 0.0 | 1.01 |
| `scan_delims_astil_simd_reg` | 156.06 ± 0.19 | 155.53 ± 0.13 | 2.4 ± 0.0 | 1.10 |
| `scan_delims_java_simd` | 304.0 ± 2.8 | 401.8 ± 3.7 | 179.01 ± 0.77 | 2.15 |
| `scan_delims_c_naive` | 404.1 ± 12.4 | 401.8 ± 7.9 | 1.2 ± 0.0 | 2.85 |
| `scan_delims_luajit` | 666.94 ± 0.43 | 664.72 ± 0.37 | 1.753 ± 0.017 | 4.71 |
| `scan_delims_java` | 670.6 ± 1.0 | 678.47 ± 0.93 | 35.962 ± 0.080 | 4.74 |
| `scan_delims_js_bun` | 944.6 ± 4.7 | 948.0 ± 4.6 | 30.653 ± 0.065 | 6.67 |
| `scan_delims_astil_naive_reg` | 1091.69 ± 0.49 | 1090.87 ± 0.32 | 2.419 ± 0.032 | 7.71 |
| `scan_delims_cl_sbcl` | 1144.1 ± 19.2 | 1140.9 ± 19.1 | 40.350 ± 0.024 | 8.08 |
| `scan_delims_pypy` | 1414.95 ± 0.74 | 1412.33 ± 0.81 | 31.353 ± 0.014 | 9.99 |
| `scan_delims_python` | 5033.8 ± 86.9 | 5027.9 ± 84.9 | 11.67 ± 0.11 | 35.55 |

## BINARY TREE

| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_cl_sbcl` | 0.301 ± 0.001 | 0.297 ± 0.002 | 112.478 ± 0.036 | 1.00 |
| `bin_tree_java` | 0.303 ± 0.005 | 0.379 ± 0.010 | 916.7 ± 19.9 | 1.01 |
| `bin_tree_astil_aot` | 0.315 ± 0.011 | 0.313 ± 0.008 | 25.2 ± 0.0 | 1.05 |
| `bin_tree_astil_reg` | 0.320 ± 0.003 | 0.320 ± 0.003 | 26.403 ± 0.017 | 1.06 |
| `bin_tree_js_bun_lucky` | 0.407 ± 0.011 | 0.597 ± 0.041 | 186.6 ± 20.2 | 1.35 |
| `bin_tree_zig` | 0.417 ± 3.4e-04 | 0.416 ± 3.6e-04 | 25.9 ± 0.0 | 1.38 |
| `bin_tree_clang` | 1.042 ± 0.004 | 1.040 ± 0.004 | 27.1 ± 0.0 | 3.46 |
| `bin_tree_js_bun` | 1.065 ± 0.019 | 1.249 ± 0.032 | 187.2 ± 17.3 | 3.53 |
| `bin_tree_go` | 1.118 ± 0.005 | 4.644 ± 0.036 | 45.0 ± 1.9 | 3.71 |
| `bin_tree_luajit` | 2.026 ± 0.006 | 2.024 ± 0.006 | 266.4 ± 18.3 | 6.73 |
| `bin_tree_astil_stack` | 2.191 ± 0.031 | 2.189 ± 0.030 | 25.8 ± 0.0 | 7.27 |
| `bin_tree_pypy` | 2.620 ± 0.031 | 2.616 ± 0.031 | 187.9 ± 43.3 | 8.70 |
| `bin_tree_gforth` | 4.714 ± 0.013 | 4.709 ± 0.010 | 28.053 ± 0.021 | 15.65 |
| `bin_tree_python` | 8.183 ± 0.045 | 8.177 ± 0.043 | 84.02 ± 0.14 | 27.16 |

## BINARY TREE BULK

| Command | Wall [ms] | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 142.1 ± 3.0 | 141.5 ± 3.0 | 35.9 ± 0.0 | 1.00 |
| `bin_tree_astil_aot_bulk` | 193.9 ± 4.5 | 193.2 ± 4.5 | 25.2 ± 0.0 | 1.36 |
| `bin_tree_astil_reg_bulk` | 203.1 ± 1.2 | 202.5 ± 1.2 | 26.397 ± 0.014 | 1.43 |
| `bin_tree_go_bulk` | 369.4 ± 9.4 | 720.5 ± 7.0 | 56.7 ± 3.8 | 2.60 |
| `bin_tree_astil_stack_bulk` | 1606.4 ± 4.7 | 1603.8 ± 2.5 | 25.8 ± 0.0 | 11.30 |
| `bin_tree_gforth_bulk` | 1618.8 ± 12.1 | 1616.6 ± 11.9 | 36.89 ± 0.16 | 11.39 |
