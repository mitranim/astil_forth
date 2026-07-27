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
- `_jit`     -- Astil reg-CC in JIT mode.
- `_stack`   -- Astil stack-CC in JIT mode.

Each `_jit` and `_stack` benchmark includes the cost of bootstrapping the entire language first. Reg-CC takes longer to bootstrap because it has more features.

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

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `none_astil_jit` | 1.163 ± 0.065 | 891.2 ± 40.6 | 1.419 ± 0.013 | 1.00 |
| `none_astil_stack` | 1.70 ± 0.33 | 1079.4 ± 258.2 | 1.3 ± 0.0 | 1.46 |

## BASELINE

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 1.497 ± 0.052 | 791.4 ± 33.8 | 1.4 ± 0.0 | 1.00 |
| `baseline_gforth` | 2.63 ± 0.16 | 1903.8 ± 113.9 | 2.066 ± 0.017 | 1.75 |
| `baseline_astil_stack` | 3.138 ± 0.090 | 2847.0 ± 90.3 | 1.7 ± 0.0 | 2.10 |
| `baseline_js_bun` | 7.05 ± 0.20 | 5648.0 ± 188.1 | 19.156 ± 0.077 | 4.71 |
| `baseline_cl_sbcl` | 12.21 ± 0.27 | 10293.0 ± 333.2 | 39.188 ± 0.035 | 8.16 |
| `baseline_pypy` | 14.12 ± 0.16 | 12893.2 ± 182.8 | 28.019 ± 0.081 | 9.43 |
| `baseline_astil_jit` | 15.1 ± 1.7 | 14665.8 ± 1647.0 | 2.4 ± 0.0 | 10.08 |
| `baseline_python` | 15.87 ± 0.20 | 13772.8 ± 123.4 | 11.44 ± 0.12 | 10.60 |
| `baseline_java` | 37.67 ± 0.91 | 40118.2 ± 998.6 | 34.900 ± 0.094 | 25.17 |

## BUBBLE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 287.7 ± 1.6 | 287.1 ± 1.6 | 1.3 ± 0.0 | 1.00 |
| `bubble_js_bun` | 414.4 ± 9.7 | 416.0 ± 9.5 | 28.634 ± 0.041 | 1.44 |
| `bubble_astil_aot` | 456.20 ± 0.67 | 455.39 ± 0.62 | 1.3 ± 0.0 | 1.59 |
| `bubble_astil_jit` | 467.92 ± 0.58 | 467.25 ± 0.62 | 2.597 ± 0.007 | 1.63 |
| `bubble_cl_sbcl` | 490.7 ± 12.4 | 487.6 ± 12.2 | 40.334 ± 0.039 | 1.71 |
| `bubble_java` | 522.1 ± 8.6 | 525.0 ± 8.6 | 35.89 ± 0.11 | 1.82 |
| `bubble_luajit` | 569.8 ± 15.3 | 568.3 ± 15.5 | 1.975 ± 0.014 | 1.98 |
| `bubble_pypy` | 1135.4 ± 1.0 | 1133.45 ± 0.83 | 31.35 ± 0.10 | 3.95 |
| `bubble_astil_stack` | 1471.7 ± 27.8 | 1470.9 ± 27.7 | 2.028 ± 0.074 | 5.12 |
| `bubble_gforth` | 3765.1 ± 185.9 | 3763.6 ± 185.8 | 2.322 ± 0.028 | 13.09 |
| `bubble_python` | 26698.0 ± 205.6 | 26695.6 ± 205.6 | 12.922 ± 0.027 | 92.81 |

## PRIME SIEVE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 144.84 ± 0.48 | 144.23 ± 0.43 | 1.163 ± 0.014 | 1.00 |
| `sieve_cl_sbcl` | 178.0 ± 2.5 | 175.6 ± 2.6 | 40.144 ± 0.028 | 1.23 |
| `sieve_js_bun` | 212.0 ± 3.3 | 211.8 ± 3.5 | 27.322 ± 0.046 | 1.46 |
| `sieve_astil_aot` | 220.6 ± 8.1 | 220.0 ± 8.1 | 1.2 ± 0.0 | 1.52 |
| `sieve_astil_jit` | 228.7 ± 6.4 | 228.1 ± 6.4 | 2.4 ± 0.0 | 1.58 |
| `sieve_luajit` | 245.3 ± 2.6 | 243.6 ± 2.7 | 1.6 ± 0.0 | 1.69 |
| `sieve_java` | 246.1 ± 8.2 | 254.0 ± 8.7 | 36.131 ± 0.073 | 1.70 |
| `sieve_pypy` | 582.66 ± 0.59 | 580.93 ± 0.57 | 31.68 ± 0.11 | 4.02 |
| `sieve_astil_stack` | 695.9 ± 5.9 | 695.1 ± 5.8 | 1.7 ± 0.0 | 4.80 |
| `sieve_gforth` | 1755.5 ± 36.6 | 1754.0 ± 36.6 | 2.084 ± 0.032 | 12.12 |
| `sieve_python` | 15301.1 ± 57.3 | 15298.7 ± 57.1 | 11.719 ± 0.046 | 105.64 |

## REVERSE STRING

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 96.56 ± 0.49 | 95.96 ± 0.41 | 1.2 ± 0.0 | 1.00 |
| `reverse_string_java` | 135.48 ± 0.31 | 142.76 ± 0.23 | 36.10 ± 0.17 | 1.40 |
| `reverse_string_js_bun` | 220.3 ± 1.3 | 224.0 ± 1.3 | 30.84 ± 0.11 | 2.28 |
| `reverse_string_astil_aot` | 233.0 ± 5.8 | 232.3 ± 5.9 | 1.2 ± 0.0 | 2.41 |
| `reverse_string_astil_jit` | 243.5 ± 5.9 | 242.9 ± 5.9 | 2.4 ± 0.0 | 2.52 |
| `reverse_string_luajit` | 322.57 ± 0.28 | 320.89 ± 0.52 | 1.7 ± 0.0 | 3.34 |
| `reverse_string_cl_sbcl` | 705.6 ± 25.6 | 702.8 ± 25.6 | 39.9 ± 0.0 | 7.31 |
| `reverse_string_pypy` | 914.6 ± 7.8 | 912.8 ± 7.7 | 32.281 ± 0.051 | 9.47 |
| `reverse_string_astil_stack` | 1734.7 ± 40.2 | 1733.8 ± 40.2 | 1.7 ± 0.0 | 17.97 |
| `reverse_string_gforth` | 4175.0 ± 4.5 | 4173.5 ± 4.5 | 2.087 ± 0.030 | 43.24 |
| `reverse_string_python` | 13074.4 ± 183.7 | 13072.0 ± 183.6 | 11.63 ± 0.13 | 135.41 |

## FIB_LOOP

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_clang` | 100.56 ± 0.47 | 100.00 ± 0.39 | 1.153 ± 0.017 | 1.00 |
| `fib_loop_astil_asm_aot` | 102.72 ± 0.39 | 102.14 ± 0.33 | 1.191 ± 0.017 | 1.02 |
| `fib_loop_java` | 142.0 ± 1.6 | 146.4 ± 1.5 | 35.42 ± 0.14 | 1.41 |
| `fib_loop_astil_aot` | 186.7 ± 10.2 | 186.1 ± 10.2 | 1.197 ± 0.014 | 1.86 |
| `fib_loop_cl_sbcl` | 192.2 ± 3.6 | 189.7 ± 3.4 | 92.350 ± 0.030 | 1.91 |
| `fib_loop_astil_jit` | 196.1 ± 5.5 | 195.5 ± 5.4 | 2.400 ± 0.021 | 1.95 |
| `fib_loop_js_bun` | 272.12 ± 0.50 | 272.97 ± 0.32 | 27.631 ± 0.070 | 2.71 |
| `fib_loop_luajit` | 369.65 ± 0.24 | 368.05 ± 0.27 | 1.6 ± 0.0 | 3.68 |
| `fib_loop_pypy` | 798.1 ± 3.4 | 796.3 ± 3.6 | 31.056 ± 0.050 | 7.94 |
| `fib_loop_astil_stack` | 1145.6 ± 29.2 | 1144.8 ± 29.3 | 1.744 ± 0.021 | 11.39 |
| `fib_loop_gforth` | 2304.2 ± 115.2 | 2302.7 ± 115.4 | 2.091 ± 0.028 | 22.91 |
| `fib_loop_python` | 12785.5 ± 101.3 | 12783.1 ± 101.3 | 11.59 ± 0.15 | 127.14 |

## FIB_LOOP_BIG

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 116.2 ± 1.3 | 115.6 ± 1.2 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 128.08 ± 0.40 | 127.53 ± 0.36 | 1.2 ± 0.0 | 1.10 |
| `fib_loop_big_astil_asm_jit` | 146.1 ± 5.2 | 145.5 ± 5.2 | 2.4 ± 0.0 | 1.26 |
| `fib_loop_big_astil_aot` | 262.7 ± 6.7 | 262.0 ± 6.7 | 1.2 ± 0.0 | 2.26 |
| `fib_loop_big_astil_jit` | 272.4 ± 7.7 | 271.8 ± 7.7 | 2.4 ± 0.0 | 2.34 |
| `fib_loop_big_cl_sbcl` | 1973.5 ± 3.3 | 1972.4 ± 3.1 | 96.741 ± 0.079 | 16.98 |
| `fib_loop_big_java` | 2491.1 ± 5.5 | 2564.1 ± 4.3 | 455.5 ± 65.8 | 21.44 |
| `fib_loop_big_pypy` | 2727.2 ± 10.4 | 2725.2 ± 10.6 | 35.98 ± 0.14 | 23.47 |
| `fib_loop_big_js_bun` | 3241.6 ± 9.5 | 3260.9 ± 10.8 | 60.884 ± 0.095 | 27.90 |
| `fib_loop_big_python` | 13544.0 ± 116.2 | 13541.4 ± 116.1 | 11.662 ± 0.054 | 116.57 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 155.9 ± 5.3 | 155.4 ± 5.3 | 1.153 ± 0.017 | 1.00 |
| `fib_rec_java` | 196.1 ± 1.1 | 199.2 ± 1.2 | 35.34 ± 0.12 | 1.26 |
| `fib_rec_astil_aot` | 227.9 ± 1.2 | 227.3 ± 1.2 | 1.2 ± 0.0 | 1.46 |
| `fib_rec_astil_jit` | 243.9 ± 9.9 | 243.2 ± 9.9 | 2.419 ± 0.026 | 1.56 |
| `fib_rec_js_bun` | 262.6 ± 9.5 | 261.8 ± 9.5 | 26.047 ± 0.025 | 1.68 |
| `fib_rec_luajit` | 275.0 ± 4.1 | 273.4 ± 4.0 | 1.669 ± 0.017 | 1.76 |
| `fib_rec_cl_sbcl` | 319.93 ± 0.55 | 317.13 ± 0.66 | 39.016 ± 0.022 | 2.05 |
| `fib_rec_pypy` | 464.1 ± 2.3 | 462.5 ± 2.3 | 46.59 ± 0.11 | 2.98 |
| `fib_rec_astil_stack` | 987.1 ± 11.9 | 986.5 ± 11.9 | 1.788 ± 0.021 | 6.33 |
| `fib_rec_gforth` | 1418.4 ± 9.3 | 1417.0 ± 9.3 | 2.084 ± 0.030 | 9.10 |
| `fib_rec_python` | 6536.2 ± 68.1 | 6533.6 ± 68.1 | 11.597 ± 0.028 | 41.92 |

## CONST FOLD

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 136.1 ± 3.0 | 135.5 ± 3.0 | 1.2 ± 0.0 | 1.00 |
| `const_fold_folded_astil_jit` | 146.74 ± 0.14 | 146.20 ± 0.12 | 2.4 ± 0.0 | 1.08 |
| `const_fold_runtime_astil_aot` | 2702.0 ± 6.9 | 2701.1 ± 6.9 | 1.2 ± 0.0 | 19.85 |
| `const_fold_runtime_astil_jit` | 2717.4 ± 10.9 | 2716.5 ± 10.8 | 2.416 ± 0.021 | 19.97 |

## FNV-1A 64

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_clang` | 134.56 ± 0.39 | 133.98 ± 0.32 | 1.228 ± 0.014 | 1.00 |
| `fnv1a64_astil_aot` | 134.73 ± 0.32 | 134.15 ± 0.27 | 1.206 ± 0.017 | 1.00 |
| `fnv1a64_cl_sbcl` | 149.36 ± 0.90 | 147.06 ± 0.81 | 39.7 ± 0.0 | 1.11 |
| `fnv1a64_astil_jit` | 150.2 ± 4.4 | 149.6 ± 4.4 | 2.434 ± 0.026 | 1.12 |
| `fnv1a64_java` | 174.05 ± 0.26 | 180.67 ± 0.72 | 35.85 ± 0.10 | 1.29 |
| `fnv1a64_pypy` | 277.22 ± 0.50 | 275.31 ± 0.45 | 31.122 ± 0.085 | 2.06 |
| `fnv1a64_gforth` | 535.3 ± 10.7 | 534.0 ± 10.7 | 2.141 ± 0.025 | 3.98 |
| `fnv1a64_astil_stack` | 605.1 ± 21.7 | 604.4 ± 21.6 | 1.834 ± 0.021 | 4.50 |
| `fnv1a64_luajit` | 1100.09 ± 0.48 | 1098.43 ± 0.35 | 1.816 ± 0.017 | 8.18 |
| `fnv1a64_js_bun` | 1563.2 ± 44.3 | 1564.9 ± 44.5 | 29.097 ± 0.097 | 11.62 |
| `fnv1a64_python` | 9762.0 ± 22.4 | 9759.6 ± 22.4 | 11.797 ± 0.031 | 72.55 |

## SCAN DELIMS

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 141.17 ± 0.72 | 140.59 ± 0.72 | 1.216 ± 0.017 | 1.00 |
| `scan_delims_astil_simd_aot` | 143.8 ± 1.1 | 143.2 ± 1.1 | 1.194 ± 0.014 | 1.02 |
| `scan_delims_astil_simd_jit` | 157.13 ± 0.67 | 156.57 ± 0.65 | 2.428 ± 0.024 | 1.11 |
| `scan_delims_java_simd` | 301.5 ± 1.0 | 398.2 ± 1.9 | 178.68 ± 0.63 | 2.14 |
| `scan_delims_c_naive` | 397.56 ± 0.26 | 396.86 ± 0.18 | 1.209 ± 0.014 | 2.82 |
| `scan_delims_astil_cell_aot` | 572.78 ± 0.34 | 572.10 ± 0.26 | 1.200 ± 0.017 | 4.06 |
| `scan_delims_astil_cell_jit` | 585.54 ± 0.50 | 584.85 ± 0.44 | 2.450 ± 0.026 | 4.15 |
| `scan_delims_luajit` | 666.16 ± 0.21 | 664.48 ± 0.15 | 1.7 ± 0.0 | 4.72 |
| `scan_delims_java` | 670.6 ± 1.1 | 678.9 ± 1.3 | 35.925 ± 0.074 | 4.75 |
| `scan_delims_js_bun` | 948.0 ± 4.0 | 952.2 ± 4.0 | 30.619 ± 0.050 | 6.72 |
| `scan_delims_astil_naive_aot` | 1078.62 ± 0.31 | 1077.91 ± 0.31 | 1.2 ± 0.0 | 7.64 |
| `scan_delims_astil_naive_jit` | 1091.44 ± 0.47 | 1090.72 ± 0.45 | 2.4 ± 0.0 | 7.73 |
| `scan_delims_cl_sbcl` | 1150.0 ± 11.2 | 1147.1 ± 11.3 | 40.356 ± 0.055 | 8.15 |
| `scan_delims_pypy` | 1413.6 ± 1.1 | 1411.4 ± 1.1 | 31.266 ± 0.067 | 10.01 |
| `scan_delims_python` | 5062.7 ± 111.4 | 5060.5 ± 111.5 | 11.64 ± 0.13 | 35.86 |

## BINARY TREE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_java` | 295.5 ± 4.0 | 370.1 ± 14.9 | 913.2 ± 30.8 | 1.00 |
| `bin_tree_cl_sbcl` | 300.33 ± 0.48 | 297.12 ± 0.44 | 112.522 ± 0.048 | 1.02 |
| `bin_tree_astil_aot` | 309.8 ± 2.9 | 308.7 ± 2.9 | 25.222 ± 0.017 | 1.05 |
| `bin_tree_astil_jit` | 332.9 ± 17.7 | 332.2 ± 17.6 | 26.4 ± 0.0 | 1.13 |
| `bin_tree_js_bun_lucky` | 397.6 ± 14.0 | 577.5 ± 48.4 | 187.9 ± 24.1 | 1.35 |
| `bin_tree_zig` | 416.36 ± 0.59 | 415.70 ± 0.57 | 25.916 ± 0.021 | 1.41 |
| `bin_tree_clang` | 1045.1 ± 11.2 | 1044.3 ± 11.2 | 27.131 ± 0.014 | 3.54 |
| `bin_tree_js_bun` | 1048.7 ± 4.2 | 1232.8 ± 8.5 | 173.0 ± 6.7 | 3.55 |
| `bin_tree_go` | 1120.9 ± 4.2 | 4861.5 ± 29.8 | 45.3 ± 1.3 | 3.79 |
| `bin_tree_luajit` | 2008.9 ± 11.0 | 2007.4 ± 10.9 | 258.8 ± 40.7 | 6.80 |
| `bin_tree_astil_stack` | 2257.3 ± 34.3 | 2256.6 ± 34.3 | 25.8 ± 0.0 | 7.64 |
| `bin_tree_pypy` | 2610.9 ± 32.8 | 2608.5 ± 32.3 | 153.6 ± 25.1 | 8.83 |
| `bin_tree_gforth` | 4720.8 ± 19.2 | 4719.3 ± 19.3 | 28.047 ± 0.025 | 15.97 |
| `bin_tree_python` | 8173.3 ± 44.5 | 8171.2 ± 44.7 | 84.094 ± 0.040 | 27.66 |

## BINARY TREE BULK

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 146.5 ± 3.5 | 145.7 ± 3.5 | 35.878 ± 0.026 | 1.00 |
| `bin_tree_astil_bulk_aot` | 193.9 ± 1.6 | 193.0 ± 1.5 | 25.238 ± 0.017 | 1.32 |
| `bin_tree_astil_bulk_jit` | 208.06 ± 0.32 | 207.49 ± 0.31 | 26.456 ± 0.007 | 1.42 |
| `bin_tree_go_bulk` | 375.2 ± 15.2 | 724.1 ± 11.9 | 59.9 ± 7.6 | 2.56 |
| `bin_tree_gforth_bulk` | 1626.2 ± 12.4 | 1624.8 ± 12.4 | 36.769 ± 0.069 | 11.10 |
| `bin_tree_astil_stack_bulk` | 1630.4 ± 45.1 | 1629.7 ± 45.1 | 25.8 ± 0.0 | 11.13 |

## TCP CONNECTIONS

Measures 4096 concurrent connections with 32 one-byte request/echo exchanges per connection.

Wall time includes Python TCP driver work. After every connection closes, the driver kills and reaps the idle server; this work is also included in wall time. CPU time and peak mem/RSS measure only the server subprocess.

This benchmark is noisier than others, especially when using pthreads. Results vary more between reruns.

Results are sorted by total user+kernel CPU time.

Each implementation is measured 5 times.

| Command | Wall [ms] | User CPU [ms] | Kernel CPU [ms] | Peak mem [MiB] | CPU relative ↓ |
| --- | ---: | ---: | ---: | ---: | ---: |
| `tcp_conn_js_bun_callback` | 920.6 ± 6.1 | 93.50 ± 0.73 | 330.1 ± 5.3 | 30.65 ± 0.11 | 1.00 |
| `tcp_conn_luajit_luv_callback` | 901.2 ± 16.5 | 147.7 ± 2.8 | 277.5 ± 5.4 | 6.812 ± 0.046 | 1.00 |
| `tcp_conn_js_bun_coro` | 920.5 ± 9.5 | 155.9 ± 1.7 | 319.1 ± 6.1 | 58.6 ± 1.1 | 1.12 |
| `tcp_conn_luajit_luv_coro` | 913.6 ± 10.5 | 239.8 ± 1.1 | 291.2 ± 5.1 | 13.422 ± 0.072 | 1.25 |
| `tcp_conn_clang_pthread` | 1233.9 ± 63.1 | 59.28 ± 0.87 | 682.5 ± 6.6 | 65.231 ± 0.039 | 1.75 |
| `tcp_conn_astil_pthread_aot` | 1284.0 ± 9.1 | 62.06 ± 0.56 | 681.8 ± 3.0 | 65.841 ± 0.051 | 1.76 |
| `tcp_conn_zig_pthread` | 1331.8 ± 14.7 | 103.42 ± 0.76 | 693.0 ± 5.8 | 65.959 ± 0.039 | 1.88 |
| `tcp_conn_astil_pthread_jit` | 1262.0 ± 11.9 | 121.0 ± 5.2 | 675.7 ± 3.3 | 67.87 ± 0.12 | 1.88 |
| `tcp_conn_python_asyncio` | 1150.7 ± 7.4 | 637.0 ± 3.1 | 359.1 ± 2.7 | 44.72 ± 0.40 | 2.35 |
| `tcp_conn_java_thread` | 1277.6 ± 118.3 | 559.4 ± 27.6 | 951.4 ± 13.3 | 550.6 ± 9.4 | 3.57 |
| `tcp_conn_cl_sbcl_thread` | 1270.8 ± 73.6 | 478.5 ± 10.7 | 1084.5 ± 171.5 | 736.0 ± 29.7 | 3.69 |
| `tcp_conn_pypy_asyncio` | 2089.5 ± 28.9 | 1254.2 ± 8.8 | 554.5 ± 27.6 | 124.4 ± 7.9 | 4.27 |
| `tcp_conn_go_goroutine` | 1240.8 ± 97.9 | 360.8 ± 3.7 | 1836.8 ± 68.6 | 18.57 ± 0.12 | 5.19 |
| `tcp_conn_java_async` | 1092.1 ± 144.1 | 704.5 ± 20.6 | 2246.2 ± 184.6 | 83.3 ± 1.1 | 6.97 |
| `tcp_conn_gforth_task` | 3757.1 ± 124.4 | 1585.7 ± 53.9 | 2098.0 ± 72.2 | 68.631 ± 0.036 | 8.70 |
| `tcp_conn_java_vthread` | 1144.9 ± 118.6 | 966.4 ± 38.7 | 4607.9 ± 111.4 | 102.5 ± 4.0 | 13.16 |
| `tcp_conn_python_thread` | 7065.3 ± 281.6 | 766.8 ± 10.9 | 47584.4 ± 2476.1 | 156.52 ± 0.11 | 114.14 |
