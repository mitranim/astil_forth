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
| `none_astil_jit` | 1.153 ± 0.046 | 867.8 ± 38.3 | 1.413 ± 0.014 | 1.00 |
| `none_astil_stack` | 2.25 ± 0.74 | 1551.0 ± 612.2 | 1.341 ± 0.017 | 1.95 |

## BASELINE

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 1.522 ± 0.087 | 793.2 ± 50.0 | 1.4 ± 0.0 | 1.00 |
| `baseline_gforth` | 2.477 ± 0.059 | 1788.6 ± 39.4 | 2.062 ± 0.016 | 1.63 |
| `baseline_astil_stack` | 3.285 ± 0.092 | 2942.2 ± 73.1 | 1.7 ± 0.0 | 2.16 |
| `baseline_js_bun` | 7.13 ± 0.34 | 5860.4 ± 299.0 | 19.137 ± 0.007 | 4.68 |
| `baseline_cl_sbcl` | 11.77 ± 0.27 | 10212.2 ± 181.5 | 39.209 ± 0.034 | 7.73 |
| `baseline_pypy` | 14.52 ± 0.30 | 13305.6 ± 322.3 | 28.028 ± 0.090 | 9.54 |
| `baseline_astil_jit` | 15.2 ± 1.8 | 14745.2 ± 1797.8 | 2.4 ± 0.0 | 9.98 |
| `baseline_python` | 16.4 ± 1.5 | 14707.2 ± 1100.7 | 11.57 ± 0.18 | 10.79 |
| `baseline_java` | 35.92 ± 0.93 | 38308.4 ± 1054.4 | 34.909 ± 0.030 | 23.60 |

## BUBBLE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 317.3 ± 4.6 | 316.5 ± 4.3 | 1.334 ± 0.014 | 1.00 |
| `bubble_js_bun` | 418.5 ± 9.1 | 420.3 ± 9.3 | 28.597 ± 0.064 | 1.32 |
| `bubble_astil_aot` | 431.4 ± 13.8 | 430.8 ± 13.9 | 1.3 ± 0.0 | 1.36 |
| `bubble_astil_jit` | 433.6 ± 18.5 | 432.7 ± 18.5 | 2.6 ± 0.0 | 1.37 |
| `bubble_cl_sbcl` | 491.3 ± 5.6 | 488.6 ± 5.5 | 40.369 ± 0.021 | 1.55 |
| `bubble_java` | 530.9 ± 2.5 | 534.4 ± 2.8 | 35.67 ± 0.10 | 1.67 |
| `bubble_luajit` | 571.0 ± 13.4 | 569.3 ± 13.2 | 1.975 ± 0.014 | 1.80 |
| `bubble_pypy` | 1135.2 ± 1.2 | 1133.3 ± 1.1 | 31.38 ± 0.16 | 3.58 |
| `bubble_astil_stack` | 1541.9 ± 16.1 | 1541.1 ± 16.1 | 2.0 ± 0.0 | 4.86 |
| `bubble_gforth` | 3848.1 ± 270.8 | 3846.7 ± 270.9 | 2.341 ± 0.039 | 12.13 |
| `bubble_python` | 26732.3 ± 90.5 | 26728.6 ± 88.6 | 12.953 ± 0.022 | 84.25 |

## PRIME SIEVE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 144.93 ± 0.15 | 144.27 ± 0.14 | 1.163 ± 0.014 | 1.00 |
| `sieve_cl_sbcl` | 178.8 ± 1.4 | 176.3 ± 1.4 | 40.2 ± 0.0 | 1.23 |
| `sieve_js_bun` | 220.1 ± 7.9 | 219.9 ± 8.0 | 27.331 ± 0.046 | 1.52 |
| `sieve_astil_aot` | 223.6 ± 2.0 | 222.9 ± 2.0 | 1.2 ± 0.0 | 1.54 |
| `sieve_astil_jit` | 236.96 ± 0.84 | 236.24 ± 0.84 | 2.4 ± 0.0 | 1.63 |
| `sieve_luajit` | 250.0 ± 1.7 | 248.6 ± 1.8 | 1.6 ± 0.0 | 1.73 |
| `sieve_java` | 251.43 ± 0.69 | 259.80 ± 0.54 | 36.17 ± 0.11 | 1.73 |
| `sieve_pypy` | 583.43 ± 0.70 | 581.62 ± 0.63 | 31.64 ± 0.17 | 4.03 |
| `sieve_astil_stack` | 703.4 ± 2.4 | 702.6 ± 2.4 | 1.7 ± 0.0 | 4.85 |
| `sieve_gforth` | 1763.3 ± 32.4 | 1762.2 ± 32.5 | 2.072 ± 0.009 | 12.17 |
| `sieve_python` | 15315.0 ± 22.4 | 15312.0 ± 22.2 | 11.794 ± 0.083 | 105.67 |

## REVERSE STRING

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 97.59 ± 0.53 | 96.96 ± 0.52 | 1.2 ± 0.0 | 1.00 |
| `reverse_string_java` | 135.14 ± 0.31 | 141.95 ± 0.27 | 36.13 ± 0.15 | 1.38 |
| `reverse_string_js_bun` | 220.53 ± 0.27 | 224.22 ± 0.19 | 30.869 ± 0.063 | 2.26 |
| `reverse_string_astil_aot` | 238.07 ± 0.81 | 237.33 ± 0.81 | 1.2 ± 0.0 | 2.44 |
| `reverse_string_astil_jit` | 249.3 ± 1.7 | 248.5 ± 1.7 | 2.4 ± 0.0 | 2.55 |
| `reverse_string_luajit` | 323.03 ± 0.66 | 321.19 ± 0.63 | 1.7 ± 0.0 | 3.31 |
| `reverse_string_cl_sbcl` | 720.8 ± 25.0 | 717.9 ± 25.0 | 39.97 ± 0.10 | 7.39 |
| `reverse_string_pypy` | 926.3 ± 4.2 | 924.4 ± 4.3 | 32.23 ± 0.14 | 9.49 |
| `reverse_string_astil_stack` | 1803.1 ± 8.3 | 1802.2 ± 8.3 | 1.7 ± 0.0 | 18.48 |
| `reverse_string_gforth` | 4174.0 ± 18.0 | 4172.7 ± 18.0 | 2.072 ± 0.014 | 42.77 |
| `reverse_string_python` | 13301.6 ± 16.9 | 13298.8 ± 17.3 | 11.63 ± 0.14 | 136.30 |

## FIB_LOOP

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_clang` | 101.02 ± 0.38 | 100.47 ± 0.36 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_astil_asm_aot` | 102.79 ± 0.29 | 102.14 ± 0.23 | 1.178 ± 0.014 | 1.02 |
| `fib_loop_java` | 144.3 ± 1.2 | 148.57 ± 0.81 | 35.57 ± 0.13 | 1.43 |
| `fib_loop_astil_aot` | 159.9 ± 1.1 | 159.2 ± 1.2 | 1.178 ± 0.014 | 1.58 |
| `fib_loop_astil_jit` | 183.3 ± 15.7 | 182.7 ± 15.7 | 2.4 ± 0.0 | 1.81 |
| `fib_loop_cl_sbcl` | 195.0 ± 1.5 | 192.3 ± 1.4 | 92.338 ± 0.039 | 1.93 |
| `fib_loop_js_bun` | 273.07 ± 0.67 | 274.0 ± 1.0 | 27.77 ± 0.13 | 2.70 |
| `fib_loop_luajit` | 371.77 ± 0.88 | 369.95 ± 0.82 | 1.6 ± 0.0 | 3.68 |
| `fib_loop_pypy` | 805.4 ± 3.0 | 803.6 ± 3.1 | 31.69 ± 0.54 | 7.97 |
| `fib_loop_astil_stack` | 1138.9 ± 31.2 | 1138.0 ± 31.2 | 1.744 ± 0.021 | 11.27 |
| `fib_loop_gforth` | 2301.8 ± 96.1 | 2300.5 ± 96.1 | 2.106 ± 0.028 | 22.79 |
| `fib_loop_python` | 12865.9 ± 100.2 | 12862.5 ± 100.4 | 11.51 ± 0.16 | 127.36 |

## FIB_LOOP_BIG

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 117.4 ± 3.5 | 116.7 ± 3.4 | 1.153 ± 0.017 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 131.6 ± 3.2 | 131.0 ± 3.3 | 1.191 ± 0.017 | 1.12 |
| `fib_loop_big_astil_asm_jit` | 145.4 ± 4.2 | 144.8 ± 4.1 | 2.416 ± 0.021 | 1.24 |
| `fib_loop_big_astil_aot` | 506.0 ± 2.3 | 505.2 ± 2.2 | 1.191 ± 0.017 | 4.31 |
| `fib_loop_big_astil_jit` | 521.3 ± 1.2 | 520.5 ± 1.3 | 2.416 ± 0.028 | 4.44 |
| `fib_loop_big_cl_sbcl` | 1991.7 ± 4.2 | 1990.7 ± 4.0 | 96.697 ± 0.032 | 16.96 |
| `fib_loop_big_java` | 2525.7 ± 11.4 | 2597.8 ± 8.6 | 480.5 ± 53.6 | 21.51 |
| `fib_loop_big_pypy` | 2742.4 ± 11.0 | 2740.5 ± 10.9 | 35.90 ± 0.12 | 23.36 |
| `fib_loop_big_js_bun` | 3280.7 ± 56.0 | 3300.5 ± 56.9 | 61.00 ± 0.11 | 27.94 |
| `fib_loop_big_python` | 13525.3 ± 44.2 | 13522.0 ± 43.0 | 11.51 ± 0.18 | 115.21 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 160.0 ± 7.2 | 159.3 ± 7.1 | 1.147 ± 0.014 | 1.00 |
| `fib_rec_java` | 195.7 ± 1.4 | 198.4 ± 1.4 | 35.33 ± 0.13 | 1.22 |
| `fib_rec_astil_aot` | 234.9 ± 9.5 | 234.2 ± 9.5 | 1.184 ± 0.017 | 1.47 |
| `fib_rec_astil_jit` | 246.5 ± 2.8 | 245.7 ± 2.8 | 2.403 ± 0.020 | 1.54 |
| `fib_rec_js_bun` | 259.7 ± 3.4 | 259.0 ± 3.4 | 26.066 ± 0.049 | 1.62 |
| `fib_rec_luajit` | 276.8 ± 6.4 | 275.3 ± 6.4 | 1.669 ± 0.017 | 1.73 |
| `fib_rec_cl_sbcl` | 329.7 ± 8.1 | 326.9 ± 7.9 | 38.997 ± 0.023 | 2.06 |
| `fib_rec_pypy` | 465.2 ± 1.6 | 463.5 ± 1.6 | 46.68 ± 0.18 | 2.91 |
| `fib_rec_astil_stack` | 999.4 ± 20.1 | 998.7 ± 20.1 | 1.778 ± 0.026 | 6.25 |
| `fib_rec_gforth` | 1414.3 ± 12.4 | 1412.8 ± 12.6 | 2.122 ± 0.068 | 8.84 |
| `fib_rec_python` | 6534.3 ± 36.1 | 6531.7 ± 36.0 | 11.694 ± 0.026 | 40.84 |

## CONST FOLD

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 138.5 ± 3.6 | 137.8 ± 3.6 | 1.184 ± 0.017 | 1.00 |
| `const_fold_folded_astil_jit` | 148.5 ± 1.8 | 147.9 ± 1.7 | 2.394 ± 0.007 | 1.07 |
| `const_fold_runtime_astil_aot` | 2719.3 ± 4.0 | 2718.4 ± 4.0 | 1.184 ± 0.017 | 19.64 |
| `const_fold_runtime_astil_jit` | 2725.03 ± 0.72 | 2724.16 ± 0.72 | 2.394 ± 0.007 | 19.68 |

## FNV-1A 64

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_astil_aot` | 135.26 ± 0.36 | 134.64 ± 0.38 | 1.200 ± 0.017 | 1.00 |
| `fnv1a64_clang` | 135.28 ± 0.51 | 134.67 ± 0.45 | 1.2 ± 0.0 | 1.00 |
| `fnv1a64_astil_jit` | 147.23 ± 0.14 | 146.61 ± 0.13 | 2.434 ± 0.026 | 1.09 |
| `fnv1a64_cl_sbcl` | 147.24 ± 0.19 | 144.81 ± 0.12 | 39.681 ± 0.028 | 1.09 |
| `fnv1a64_java` | 172.7 ± 1.3 | 179.0 ± 1.3 | 35.928 ± 0.089 | 1.28 |
| `fnv1a64_pypy` | 278.24 ± 0.94 | 276.52 ± 0.94 | 31.19 ± 0.20 | 2.06 |
| `fnv1a64_gforth` | 527.3 ± 7.4 | 525.9 ± 7.5 | 2.131 ± 0.014 | 3.90 |
| `fnv1a64_astil_stack` | 602.2 ± 21.9 | 601.5 ± 21.9 | 1.8 ± 0.0 | 4.45 |
| `fnv1a64_luajit` | 1109.8 ± 7.6 | 1107.5 ± 6.9 | 1.803 ± 0.014 | 8.21 |
| `fnv1a64_js_bun` | 1542.1 ± 21.5 | 1544.4 ± 20.9 | 29.084 ± 0.079 | 11.40 |
| `fnv1a64_python` | 9779.9 ± 7.8 | 9777.2 ± 7.7 | 11.73 ± 0.18 | 72.31 |

## SCAN DELIMS

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 142.4 ± 2.5 | 141.7 ± 2.5 | 1.2 ± 0.0 | 1.00 |
| `scan_delims_astil_simd_aot` | 144.6 ± 1.6 | 143.9 ± 1.5 | 1.2 ± 0.0 | 1.02 |
| `scan_delims_astil_simd_jit` | 158.88 ± 0.36 | 158.22 ± 0.37 | 2.4 ± 0.0 | 1.12 |
| `scan_delims_java_simd` | 300.9 ± 1.4 | 399.0 ± 1.9 | 178.61 ± 0.68 | 2.11 |
| `scan_delims_c_naive` | 398.93 ± 0.33 | 398.08 ± 0.30 | 1.2 ± 0.0 | 2.80 |
| `scan_delims_luajit` | 668.8 ± 1.1 | 667.0 ± 1.2 | 1.7 ± 0.0 | 4.70 |
| `scan_delims_java` | 672.8 ± 3.5 | 680.1 ± 3.4 | 36.55 ± 0.76 | 4.72 |
| `scan_delims_js_bun` | 940.9 ± 10.1 | 944.9 ± 10.2 | 30.625 ± 0.069 | 6.61 |
| `scan_delims_astil_naive_jit` | 1095.0 ± 1.7 | 1094.2 ± 1.7 | 2.409 ± 0.007 | 7.69 |
| `scan_delims_cl_sbcl` | 1139.3 ± 21.3 | 1136.4 ± 21.1 | 40.337 ± 0.034 | 8.00 |
| `scan_delims_pypy` | 1415.0 ± 1.1 | 1413.0 ± 1.1 | 31.28 ± 0.10 | 9.94 |
| `scan_delims_python` | 4987.5 ± 70.7 | 4984.8 ± 70.6 | 11.68 ± 0.13 | 35.02 |

## BINARY TREE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_cl_sbcl` | 300.92 ± 0.67 | 297.63 ± 0.55 | 112.509 ± 0.032 | 1.00 |
| `bin_tree_java` | 308.0 ± 1.4 | 387.8 ± 7.5 | 908.8 ± 28.5 | 1.02 |
| `bin_tree_astil_aot` | 318.3 ± 2.7 | 317.0 ± 2.8 | 25.2 ± 0.0 | 1.06 |
| `bin_tree_astil_jit` | 330.6 ± 5.4 | 329.9 ± 5.4 | 26.394 ± 0.007 | 1.10 |
| `bin_tree_js_bun_lucky` | 402.9 ± 8.5 | 596.6 ± 21.2 | 189.0 ± 15.2 | 1.34 |
| `bin_tree_zig` | 417.32 ± 0.81 | 416.57 ± 0.79 | 25.934 ± 0.026 | 1.39 |
| `bin_tree_js_bun` | 1051.0 ± 6.7 | 1231.6 ± 20.5 | 186.5 ± 16.5 | 3.49 |
| `bin_tree_clang` | 1071.3 ± 27.6 | 1063.8 ± 18.6 | 27.1 ± 0.0 | 3.56 |
| `bin_tree_go` | 1125.2 ± 4.7 | 4801.9 ± 57.5 | 44.08 ± 0.89 | 3.74 |
| `bin_tree_luajit` | 2031.9 ± 6.7 | 2030.4 ± 6.8 | 273.3 ± 22.7 | 6.75 |
| `bin_tree_astil_stack` | 2135.0 ± 68.2 | 2134.3 ± 68.1 | 25.8 ± 0.0 | 7.09 |
| `bin_tree_pypy` | 2595.8 ± 16.3 | 2592.8 ± 16.4 | 181.0 ± 25.9 | 8.63 |
| `bin_tree_gforth` | 4713.5 ± 26.5 | 4712.1 ± 26.7 | 28.050 ± 0.026 | 15.66 |
| `bin_tree_python` | 8208.7 ± 10.1 | 8206.2 ± 10.1 | 84.06 ± 0.16 | 27.28 |

## BINARY TREE BULK

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 145.6 ± 3.5 | 144.8 ± 3.5 | 35.878 ± 0.042 | 1.00 |
| `bin_tree_astil_bulk_aot` | 193.3 ± 2.8 | 192.4 ± 2.8 | 25.2 ± 0.0 | 1.33 |
| `bin_tree_astil_bulk_jit` | 207.5 ± 2.7 | 206.8 ± 2.6 | 26.4 ± 0.0 | 1.42 |
| `bin_tree_go_bulk` | 376.2 ± 10.9 | 722.5 ± 13.2 | 57.5 ± 3.8 | 2.58 |
| `bin_tree_gforth_bulk` | 1598.6 ± 15.5 | 1597.2 ± 15.5 | 36.741 ± 0.009 | 10.98 |
| `bin_tree_astil_stack_bulk` | 1599.0 ± 17.9 | 1598.4 ± 17.9 | 25.8 ± 0.0 | 10.98 |

## TCP CONNECTIONS

Measures 4096 concurrent connections with 32 one-byte request/echo exchanges per connection.

Wall time includes Python TCP driver work. After every connection closes, the driver kills and reaps the idle server; this work is also included in wall time. CPU time and peak mem/RSS measure only the server subprocess. Each implementation is measured 5 times.

This benchmark is noisier than others, especially when using pthreads. Results vary more between reruns.

Results are sorted by total user+kernel CPU time.

| Command | Wall [ms] | User CPU [ms] | Kernel CPU [ms] | Peak mem [MiB] | CPU relative ↓ |
| --- | ---: | ---: | ---: | ---: | ---: |
| `tcp_conn_js_bun_callback` | 928.9 ± 15.4 | 95.0 ± 1.2 | 334.8 ± 4.8 | 30.53 ± 0.11 | 1.00 |
| `tcp_conn_luajit_luv_callback` | 911.2 ± 13.7 | 151.1 ± 2.0 | 279.7 ± 2.8 | 6.89 ± 0.20 | 1.00 |
| `tcp_conn_luajit_luv_coro` | 947.9 ± 31.5 | 246.8 ± 9.1 | 296.3 ± 6.2 | 13.41 ± 0.14 | 1.26 |
| `tcp_conn_astil_pthread_aot` | 1309.7 ± 19.0 | 63.19 ± 0.60 | 720.0 ± 16.0 | 65.847 ± 0.040 | 1.82 |
| `tcp_conn_astil_pthread_jit` | 1272.5 ± 14.3 | 124.7 ± 4.0 | 696.1 ± 2.1 | 67.84 ± 0.11 | 1.91 |
| `tcp_conn_zig_pthread` | 1306.3 ± 117.2 | 107.1 ± 2.4 | 731.7 ± 27.5 | 65.975 ± 0.061 | 1.95 |
| `tcp_conn_clang_pthread` | 1228.6 ± 195.3 | 65.9 ± 14.4 | 914.2 ± 442.3 | 65.25 ± 0.12 | 2.28 |
| `tcp_conn_python_asyncio` | 1141.1 ± 33.7 | 632.7 ± 3.3 | 366.6 ± 10.9 | 44.17 ± 0.35 | 2.33 |
| `tcp_conn_java_thread` | 1225.3 ± 102.0 | 533.7 ± 19.1 | 1031.6 ± 39.1 | 547.6 ± 15.8 | 3.64 |
| `tcp_conn_pypy_asyncio` | 2128.1 ± 136.7 | 1248.2 ± 48.0 | 555.0 ± 8.6 | 123.9 ± 7.1 | 4.20 |
| `tcp_conn_cl_sbcl_thread` | 1457.5 ± 84.2 | 560.1 ± 63.5 | 1313.9 ± 197.6 | 1347.5 ± 585.3 | 4.36 |
| `tcp_conn_go_goroutine` | 1274.7 ± 99.8 | 365.8 ± 6.3 | 1904.1 ± 79.9 | 18.84 ± 0.50 | 5.28 |
| `tcp_conn_java_async` | 984.9 ± 40.1 | 739.8 ± 24.3 | 2573.5 ± 274.5 | 85.1 ± 2.8 | 7.71 |
| `tcp_conn_gforth_task` | 3848.0 ± 110.6 | 1605.4 ± 45.8 | 2160.9 ± 66.3 | 68.672 ± 0.058 | 8.76 |
| `tcp_conn_java_vthread` | 1085.7 ± 132.9 | 979.8 ± 86.7 | 4623.6 ± 61.5 | 104.2 ± 2.2 | 13.04 |
