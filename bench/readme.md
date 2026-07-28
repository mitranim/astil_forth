Microbenchmarks comparing:
- Register-CC version of Astil Forth in AOT mode.
- Register-CC version of Astil Forth in JIT mode.
- Stack-CC version of Astil Forth in JIT mode.
- Gforth.
- C (Clang).
- JS (Bun).
- Lua (LuaJIT).
- Common Lisp (SBCL).
- Erlang/BEAM.
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
- The suite records wall time, subprocess CPU time, and peak RSS. GC-based engines can spend substantial CPU on background threads.
- In many benchmarks, _startup time skews the measurement_. Adjust them by the "baseline" metrics when comparing.
- Serial Erlang CPU and cat rows, plus `baseline_erlang_single`, use one BEAM scheduler (`+S 1:1`). Erlang TCP rows and `baseline_erlang_default` use the default scheduler count.
- The functional `bubble_erlang_array` candidate is omitted because one fixed-workload validation exceeded the 60-second limit. The mutable `atomics` variant is retained.
- Dominated Erlang variants were measured then removed: persistent-array sieve, bulk-match delimiter scan, `file:copy/2` cat, and active-once TCP.
- Erlang TCP rows use `+sbwt none`; it dramatically reduced active-N server CPU without changing the default scheduler count.

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

Erlang/OTP 28 [erts-16.4] [source] [64-bit] [smp:12:12] [ds:12:12:10] [async-threads:1] [jit] [dtrace]

[pypy 7.3.17 with gcc apple llvm 16.0.0 (clang-1600.0.26.4)]
python 3.10.14 (39dc8d3c85a7, nov 09 2024, 22:49:03)

python 3.14.4

Erlang/OTP 28 [erts-16.4] [source] [64-bit] [smp:12:12] [ds:12:12:10] [async-threads:1] [jit] [dtrace]
```

## NONE

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `none_astil_jit` | 1.215 ± 0.063 | 941.0 ± 85.1 | 1.425 ± 0.017 | 1.00 |
| `none_astil_stack` | 1.78 ± 0.27 | 1210.8 ± 203.5 | 1.331 ± 0.007 | 1.46 |

## BASELINE

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 1.87 ± 0.25 | 869.2 ± 56.4 | 1.413 ± 0.014 | 1.00 |
| `baseline_gforth` | 2.67 ± 0.12 | 1872.8 ± 108.0 | 2.066 ± 0.013 | 1.43 |
| `baseline_astil_stack` | 3.11 ± 0.14 | 2840.8 ± 113.4 | 1.753 ± 0.026 | 1.66 |
| `baseline_js_bun` | 7.432 ± 0.088 | 5920.8 ± 188.8 | 19.222 ± 0.093 | 3.98 |
| `baseline_cl_sbcl` | 12.02 ± 0.39 | 10127.2 ± 424.9 | 39.228 ± 0.045 | 6.44 |
| `baseline_pypy` | 14.50 ± 0.24 | 13256.6 ± 216.2 | 27.96 ± 0.11 | 7.77 |
| `baseline_astil_jit` | 14.8 ± 1.9 | 14486.8 ± 1819.1 | 2.400 ± 0.021 | 7.94 |
| `baseline_python` | 15.69 ± 0.22 | 13790.6 ± 188.7 | 11.588 ± 0.041 | 8.41 |
| `baseline_java` | 34.86 ± 0.23 | 37386.0 ± 227.5 | 34.959 ± 0.092 | 18.68 |
| `baseline_erlang_single` | 69.18 ± 0.89 | 68877.4 ± 537.7 | 47.60 ± 0.62 | 37.07 |
| `baseline_erlang_default` | 70.44 ± 0.93 | 213590.0 ± 9984.5 | 58.50 ± 0.56 | 37.75 |

## BUBBLE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 283.46 ± 0.65 | 282.86 ± 0.61 | 1.347 ± 0.017 | 1.00 |
| `bubble_js_bun` | 417.9 ± 12.0 | 419.7 ± 12.0 | 28.613 ± 0.066 | 1.47 |
| `bubble_astil_aot` | 455.04 ± 0.64 | 454.34 ± 0.61 | 1.369 ± 0.014 | 1.61 |
| `bubble_astil_jit` | 467.28 ± 0.57 | 466.44 ± 0.51 | 2.612 ± 0.026 | 1.65 |
| `bubble_cl_sbcl` | 486.3 ± 12.2 | 483.5 ± 12.2 | 40.347 ± 0.007 | 1.72 |
| `bubble_java` | 523.8 ± 7.0 | 527.2 ± 6.7 | 35.77 ± 0.15 | 1.85 |
| `bubble_luajit` | 569.8 ± 15.9 | 568.1 ± 15.9 | 2.0 ± 0.0 | 2.01 |
| `bubble_pypy` | 1135.00 ± 0.85 | 1133.22 ± 0.81 | 31.34 ± 0.15 | 4.00 |
| `bubble_astil_stack` | 1502.7 ± 41.1 | 1501.8 ± 41.1 | 2.0 ± 0.0 | 5.30 |
| `bubble_gforth` | 3645.4 ± 41.0 | 3643.7 ± 41.1 | 2.319 ± 0.026 | 12.86 |
| `bubble_erlang_atomics` | 8963.9 ± 30.0 | 7492.6 ± 30.0 | 48.04 ± 0.23 | 31.62 |
| `bubble_python` | 26737.5 ± 93.8 | 26734.0 ± 93.8 | 13.03 ± 0.12 | 94.32 |

## PRIME SIEVE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 152.94 ± 0.40 | 152.30 ± 0.41 | 1.2 ± 0.0 | 1.00 |
| `sieve_cl_sbcl` | 183.0 ± 4.4 | 180.8 ± 4.4 | 40.131 ± 0.034 | 1.20 |
| `sieve_js_bun` | 216.7 ± 5.5 | 216.3 ± 5.5 | 27.403 ± 0.078 | 1.42 |
| `sieve_astil_aot` | 229.38 ± 0.82 | 228.71 ± 0.84 | 1.2 ± 0.0 | 1.50 |
| `sieve_astil_jit` | 235.1 ± 8.1 | 234.4 ± 8.1 | 2.4 ± 0.0 | 1.54 |
| `sieve_luajit` | 247.0 ± 1.3 | 245.4 ± 1.2 | 1.6 ± 0.0 | 1.61 |
| `sieve_java` | 249.82 ± 0.79 | 257.76 ± 0.95 | 36.072 ± 0.050 | 1.63 |
| `sieve_pypy` | 582.88 ± 0.72 | 581.21 ± 0.77 | 31.747 ± 0.042 | 3.81 |
| `sieve_astil_stack` | 702.7 ± 2.4 | 702.0 ± 2.4 | 1.7 ± 0.0 | 4.59 |
| `sieve_gforth` | 1730.5 ± 7.6 | 1729.1 ± 7.6 | 2.087 ± 0.014 | 11.32 |
| `sieve_erlang_atomics` | 3607.4 ± 32.8 | 2852.8 ± 33.4 | 48.06 ± 0.18 | 23.59 |
| `sieve_python` | 15326.2 ± 66.2 | 15323.7 ± 66.3 | 11.67 ± 0.14 | 100.21 |

## REVERSE STRING

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 91.42 ± 0.69 | 90.83 ± 0.71 | 1.2 ± 0.0 | 1.00 |
| `reverse_string_java` | 134.21 ± 0.54 | 141.17 ± 0.79 | 36.247 ± 0.093 | 1.47 |
| `reverse_string_js_bun` | 220.83 ± 0.91 | 224.49 ± 0.90 | 30.92 ± 0.12 | 2.42 |
| `reverse_string_astil_aot` | 230.40 ± 0.61 | 229.72 ± 0.60 | 1.2 ± 0.0 | 2.52 |
| `reverse_string_astil_jit` | 246.3 ± 4.0 | 245.6 ± 4.0 | 2.4 ± 0.0 | 2.69 |
| `reverse_string_luajit` | 322.37 ± 0.40 | 320.69 ± 0.38 | 1.7 ± 0.0 | 3.53 |
| `reverse_string_cl_sbcl` | 702.9 ± 21.2 | 700.1 ± 21.2 | 39.978 ± 0.046 | 7.69 |
| `reverse_string_pypy` | 913.9 ± 5.1 | 912.2 ± 5.0 | 32.303 ± 0.082 | 10.00 |
| `reverse_string_erlang_list` | 1092.8 ± 5.1 | 943.9 ± 4.9 | 47.89 ± 0.33 | 11.95 |
| `reverse_string_astil_stack` | 1745.3 ± 34.7 | 1744.4 ± 34.6 | 1.744 ± 0.021 | 19.09 |
| `reverse_string_erlang_binary` | 3216.4 ± 4.4 | 2739.7 ± 4.9 | 47.72 ± 0.46 | 35.18 |
| `reverse_string_gforth` | 4170.2 ± 4.1 | 4168.6 ± 3.9 | 2.106 ± 0.051 | 45.61 |
| `reverse_string_python` | 13158.8 ± 80.6 | 13156.4 ± 80.6 | 11.691 ± 0.028 | 143.93 |

## FIB_LOOP

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_astil_asm_aot` | 102.67 ± 0.22 | 102.13 ± 0.19 | 1.2 ± 0.0 | 1.00 |
| `fib_loop_clang` | 117.8 ± 1.1 | 117.2 ± 1.2 | 1.1 ± 0.0 | 1.15 |
| `fib_loop_java` | 140.60 ± 0.70 | 144.62 ± 0.73 | 35.48 ± 0.12 | 1.37 |
| `fib_loop_astil_aot` | 164.03 ± 0.24 | 163.39 ± 0.22 | 1.2 ± 0.0 | 1.60 |
| `fib_loop_astil_jit` | 171.3 ± 5.1 | 170.7 ± 5.1 | 2.419 ± 0.028 | 1.67 |
| `fib_loop_cl_sbcl` | 188.83 ± 0.81 | 186.20 ± 0.63 | 92.381 ± 0.024 | 1.84 |
| `fib_loop_js_bun` | 273.70 ± 0.99 | 274.8 ± 1.3 | 27.84 ± 0.11 | 2.67 |
| `fib_loop_luajit` | 370.13 ± 0.56 | 368.48 ± 0.59 | 1.6 ± 0.0 | 3.61 |
| `fib_loop_pypy` | 804.2 ± 3.6 | 802.5 ± 3.5 | 31.09 ± 0.13 | 7.83 |
| `fib_loop_astil_stack` | 1098.1 ± 16.9 | 1097.3 ± 16.9 | 1.744 ± 0.021 | 10.70 |
| `fib_loop_erlang` | 1160.3 ± 1.1 | 794.7 ± 1.1 | 47.69 ± 0.27 | 11.30 |
| `fib_loop_gforth` | 2271.1 ± 65.4 | 2269.3 ± 65.5 | 2.122 ± 0.091 | 22.12 |
| `fib_loop_python` | 12884.3 ± 140.7 | 12881.9 ± 140.7 | 11.672 ± 0.062 | 125.50 |

## FIB_LOOP_BIG

C and Astil use `uint128`. Other languages use actual bigints.

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 112.57 ± 0.33 | 111.94 ± 0.23 | 1.147 ± 0.014 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 128.40 ± 0.45 | 127.80 ± 0.42 | 1.178 ± 0.014 | 1.14 |
| `fib_loop_big_astil_asm_jit` | 140.77 ± 0.51 | 140.17 ± 0.50 | 2.444 ± 0.049 | 1.25 |
| `fib_loop_big_astil_aot` | 258.0 ± 1.1 | 257.31 ± 0.98 | 1.2 ± 0.0 | 2.29 |
| `fib_loop_big_astil_jit` | 269.0 ± 2.2 | 268.3 ± 2.2 | 2.419 ± 0.020 | 2.39 |
| `fib_loop_big_cl_sbcl` | 1977.2 ± 1.7 | 1976.0 ± 1.6 | 96.697 ± 0.036 | 17.56 |
| `fib_loop_big_java` | 2419.2 ± 4.6 | 2492.3 ± 5.3 | 384.33 ± 0.83 | 21.49 |
| `fib_loop_big_pypy` | 2735.1 ± 12.7 | 2733.0 ± 12.8 | 35.90 ± 0.13 | 24.30 |
| `fib_loop_big_erlang` | 2753.6 ± 7.6 | 2399.9 ± 7.9 | 47.91 ± 0.55 | 24.46 |
| `fib_loop_big_js_bun` | 3243.9 ± 13.0 | 3263.7 ± 13.7 | 61.025 ± 0.050 | 28.82 |
| `fib_loop_big_python` | 13476.9 ± 15.8 | 13474.4 ± 15.7 | 11.675 ± 0.040 | 119.72 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 153.2 ± 5.5 | 152.6 ± 5.5 | 1.159 ± 0.017 | 1.00 |
| `fib_rec_java` | 196.2 ± 2.1 | 199.1 ± 2.1 | 35.33 ± 0.18 | 1.28 |
| `fib_rec_astil_aot` | 235.6 ± 5.8 | 234.9 ± 5.9 | 1.184 ± 0.017 | 1.54 |
| `fib_rec_astil_jit` | 250.1 ± 4.7 | 249.4 ± 4.7 | 2.416 ± 0.021 | 1.63 |
| `fib_rec_js_bun` | 261.8 ± 3.6 | 260.9 ± 3.7 | 26.087 ± 0.099 | 1.71 |
| `fib_rec_luajit` | 266.8 ± 2.2 | 265.2 ± 2.4 | 1.672 ± 0.022 | 1.74 |
| `fib_rec_cl_sbcl` | 334.1 ± 11.4 | 331.3 ± 11.5 | 39.000 ± 0.019 | 2.18 |
| `fib_rec_pypy` | 465.3 ± 2.3 | 463.8 ± 2.5 | 46.73 ± 0.10 | 3.04 |
| `fib_rec_erlang` | 846.85 ± 0.27 | 387.62 ± 0.93 | 47.76 ± 0.33 | 5.53 |
| `fib_rec_astil_stack` | 1021.6 ± 52.7 | 1020.9 ± 52.7 | 1.769 ± 0.026 | 6.67 |
| `fib_rec_gforth` | 1423.1 ± 18.5 | 1421.6 ± 18.4 | 2.084 ± 0.024 | 9.29 |
| `fib_rec_python` | 6546.0 ± 16.5 | 6543.5 ± 16.4 | 11.7 ± 0.0 | 42.72 |

## CONST FOLD

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 136.5 ± 3.5 | 135.9 ± 3.5 | 1.197 ± 0.014 | 1.00 |
| `const_fold_folded_astil_jit` | 154.96 ± 0.25 | 154.36 ± 0.24 | 2.4 ± 0.0 | 1.14 |
| `const_fold_runtime_astil_aot` | 2715.7 ± 1.8 | 2714.9 ± 1.7 | 1.184 ± 0.017 | 19.89 |
| `const_fold_runtime_astil_jit` | 2727.0 ± 3.4 | 2726.1 ± 3.2 | 2.438 ± 0.048 | 19.98 |

## FNV-1A 64

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_clang` | 134.96 ± 0.45 | 134.34 ± 0.38 | 1.222 ± 0.017 | 1.00 |
| `fnv1a64_astil_aot` | 139.4 ± 3.8 | 138.8 ± 3.9 | 1.2 ± 0.0 | 1.03 |
| `fnv1a64_cl_sbcl` | 147.23 ± 0.17 | 144.87 ± 0.23 | 39.653 ± 0.013 | 1.09 |
| `fnv1a64_astil_jit` | 149.0 ± 3.6 | 148.4 ± 3.6 | 2.416 ± 0.021 | 1.10 |
| `fnv1a64_java` | 172.70 ± 0.81 | 179.11 ± 0.81 | 35.90 ± 0.11 | 1.28 |
| `fnv1a64_pypy` | 277.29 ± 0.50 | 275.44 ± 0.36 | 31.20 ± 0.12 | 2.05 |
| `fnv1a64_gforth` | 524.1 ± 7.6 | 522.7 ± 7.5 | 2.147 ± 0.026 | 3.88 |
| `fnv1a64_astil_stack` | 609.8 ± 21.0 | 609.1 ± 21.1 | 1.816 ± 0.026 | 4.52 |
| `fnv1a64_luajit` | 1101.28 ± 0.54 | 1099.59 ± 0.55 | 1.809 ± 0.017 | 8.16 |
| `fnv1a64_js_bun` | 1534.6 ± 6.9 | 1536.4 ± 6.9 | 29.084 ± 0.060 | 11.37 |
| `fnv1a64_erlang` | 4251.8 ± 8.2 | 4131.1 ± 8.3 | 47.89 ± 0.33 | 31.50 |
| `fnv1a64_python` | 9776.6 ± 30.1 | 9774.1 ± 30.1 | 11.76 ± 0.14 | 72.44 |

## SCAN DELIMS

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 143.65 ± 0.80 | 143.05 ± 0.79 | 1.2 ± 0.0 | 1.00 |
| `scan_delims_astil_simd_aot` | 144.83 ± 0.45 | 144.24 ± 0.44 | 1.2 ± 0.0 | 1.01 |
| `scan_delims_astil_simd_jit` | 158.88 ± 0.15 | 158.29 ± 0.17 | 2.4 ± 0.0 | 1.11 |
| `scan_delims_java_simd` | 304.7 ± 3.6 | 401.7 ± 2.2 | 178.73 ± 0.87 | 2.12 |
| `scan_delims_c_naive` | 398.27 ± 0.46 | 397.53 ± 0.43 | 1.2 ± 0.0 | 2.77 |
| `scan_delims_astil_cell_aot` | 573.37 ± 0.25 | 572.60 ± 0.23 | 1.2 ± 0.0 | 3.99 |
| `scan_delims_astil_cell_jit` | 586.51 ± 0.41 | 585.73 ± 0.36 | 2.4 ± 0.0 | 4.08 |
| `scan_delims_luajit` | 667.05 ± 0.27 | 665.30 ± 0.16 | 1.7 ± 0.0 | 4.64 |
| `scan_delims_java` | 668.68 ± 0.72 | 676.0 ± 2.0 | 35.1 ± 2.0 | 4.66 |
| `scan_delims_js_bun` | 950.3 ± 2.2 | 954.4 ± 2.4 | 30.669 ± 0.098 | 6.62 |
| `scan_delims_astil_naive_aot` | 1079.49 ± 0.45 | 1078.79 ± 0.43 | 1.2 ± 0.0 | 7.51 |
| `scan_delims_astil_naive_jit` | 1092.65 ± 0.60 | 1091.89 ± 0.52 | 2.431 ± 0.048 | 7.61 |
| `scan_delims_cl_sbcl` | 1172.2 ± 5.2 | 1169.3 ± 5.1 | 40.303 ± 0.034 | 8.16 |
| `scan_delims_pypy` | 1414.73 ± 0.42 | 1412.52 ± 0.35 | 31.269 ± 0.042 | 9.85 |
| `scan_delims_erlang_naive` | 2145.6 ± 3.5 | 1904.6 ± 3.0 | 47.86 ± 0.37 | 14.94 |
| `scan_delims_python` | 5057.4 ± 99.0 | 5054.8 ± 99.0 | 11.59 ± 0.12 | 35.21 |

## BINARY TREE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_java` | 295.1 ± 4.0 | 369.8 ± 15.7 | 902.3 ± 29.8 | 1.00 |
| `bin_tree_cl_sbcl` | 302.89 ± 0.89 | 299.61 ± 0.89 | 112.513 ± 0.039 | 1.03 |
| `bin_tree_astil_aot` | 322.4 ± 15.0 | 321.3 ± 15.1 | 25.222 ± 0.017 | 1.09 |
| `bin_tree_astil_jit` | 337.1 ± 17.1 | 336.4 ± 17.1 | 26.4 ± 0.0 | 1.14 |
| `bin_tree_js_bun_lucky` | 402.3 ± 3.6 | 600.5 ± 8.7 | 188.3 ± 13.0 | 1.36 |
| `bin_tree_zig` | 417.32 ± 0.92 | 416.66 ± 0.92 | 25.925 ± 0.026 | 1.41 |
| `bin_tree_clang` | 1023.8 ± 13.6 | 1023.0 ± 13.6 | 27.137 ± 0.017 | 3.47 |
| `bin_tree_erlang` | 1054.0 ± 1.1 | 570.63 ± 0.97 | 177.82 ± 0.66 | 3.57 |
| `bin_tree_js_bun` | 1054.2 ± 11.0 | 1238.9 ± 30.9 | 190.3 ± 22.0 | 3.57 |
| `bin_tree_go` | 1132.7 ± 4.5 | 4935.5 ± 30.1 | 45.0 ± 2.5 | 3.84 |
| `bin_tree_luajit` | 2014.4 ± 11.4 | 2012.8 ± 11.5 | 250.2 ± 22.1 | 6.83 |
| `bin_tree_astil_stack` | 2203.3 ± 39.1 | 2202.6 ± 39.1 | 25.769 ± 0.026 | 7.47 |
| `bin_tree_pypy` | 2601.9 ± 21.4 | 2599.3 ± 21.4 | 153.6 ± 25.1 | 8.82 |
| `bin_tree_gforth` | 4713.6 ± 10.9 | 4712.1 ± 10.9 | 28.056 ± 0.014 | 15.97 |
| `bin_tree_python` | 8185.0 ± 42.9 | 8182.7 ± 42.8 | 84.1 ± 0.0 | 27.74 |

## BINARY TREE BULK

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 142.31 ± 0.61 | 141.50 ± 0.62 | 35.906 ± 0.057 | 1.00 |
| `bin_tree_astil_bulk_aot` | 195.1 ± 1.0 | 194.3 ± 1.0 | 25.244 ± 0.014 | 1.37 |
| `bin_tree_astil_bulk_jit` | 208.8 ± 1.3 | 208.2 ± 1.3 | 26.425 ± 0.026 | 1.47 |
| `bin_tree_go_bulk` | 373.7 ± 12.5 | 728.3 ± 7.7 | 54.1 ± 2.0 | 2.63 |
| `bin_tree_astil_stack_bulk` | 1612.9 ± 40.6 | 1612.1 ± 40.6 | 25.778 ± 0.026 | 11.33 |
| `bin_tree_gforth_bulk` | 1619.3 ± 14.3 | 1617.9 ± 14.2 | 38.4 ± 3.6 | 11.38 |
| `bin_tree_bulk_erlang_binary` | 1822.9 ± 20.6 | 1408.4 ± 21.4 | 137.43 ± 0.40 | 12.81 |
| `bin_tree_bulk_erlang_atomics` | 2884.8 ± 7.7 | 1838.8 ± 7.6 | 91.80 ± 0.26 | 20.27 |

## CAT SMALL

Copies a 4 KiB file, 4 KiB stdin, then the file again (12 KiB total).

| Command | Wall [ms] ↓ | User CPU [µs] | Kernel CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cat_global_small` | 2.009 ± 0.090 | 398.8 ± 30.7 | 368.0 ± 70.1 | 1.406 ± 0.022 | 1.00 |
| `cat_clang_small` | 2.31 ± 0.73 | 807.2 ± 206.3 | 769.8 ± 259.5 | 1.212 ± 0.021 | 1.15 |
| `cat_go_small` | 2.41 ± 0.34 | 1029.0 ± 225.8 | 845.6 ± 105.9 | 3.597 ± 0.053 | 1.20 |
| `cat_luajit_small` | 2.418 ± 0.099 | 750.2 ± 64.4 | 471.0 ± 50.7 | 1.5 ± 0.0 | 1.20 |
| `cat_zig_small` | 2.5 ± 1.0 | 1082.2 ± 496.8 | 678.6 ± 351.1 | 1.9 ± 0.0 | 1.24 |
| `cat_gforth_small` | 3.11 ± 0.11 | 1480.4 ± 60.1 | 520.6 ± 63.7 | 2.100 ± 0.018 | 1.55 |
| `cat_astil_aot_small` | 3.51 ± 1.00 | 1629.6 ± 413.2 | 916.4 ± 365.1 | 1.2 ± 0.0 | 1.75 |
| `cat_js_bun_small` | 10.64 ± 0.60 | 6032.6 ± 385.9 | 2729.2 ± 151.9 | 24.519 ± 0.082 | 5.30 |
| `cat_cl_sbcl_small` | 12.86 ± 0.38 | 4409.6 ± 123.9 | 6427.8 ± 233.9 | 39.641 ± 0.058 | 6.40 |
| `cat_pypy_small` | 15.26 ± 0.29 | 9565.4 ± 154.5 | 4074.8 ± 135.7 | 28.550 ± 0.085 | 7.59 |
| `cat_python_small` | 16.26 ± 0.24 | 11459.6 ± 159.8 | 2862.2 ± 164.1 | 11.713 ± 0.041 | 8.09 |
| `cat_astil_jit_small` | 29.3 ± 1.0 | 27812.2 ± 966.9 | 886.6 ± 54.8 | 2.856 ± 0.026 | 14.56 |
| `cat_java_small` | 36.97 ± 0.30 | 16685.8 ± 48.2 | 22848.2 ± 329.0 | 35.538 ± 0.088 | 18.40 |
| `cat_erlang_loop_small` | 71.60 ± 0.46 | 56161.6 ± 272.0 | 14471.8 ± 245.4 | 48.27 ± 0.19 | 35.64 |

## CAT LARGE

Copies a warmed 512 MiB file and 512 MiB stdin (1 GiB total).

| Command | Wall [ms] ↓ | User CPU [ms] | Kernel CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cat_zig_large` | 68.66 ± 0.72 | 2.48 ± 0.10 | 65.48 ± 0.64 | 1.9 ± 0.0 | 1.00 |
| `cat_astil_aot_large` | 69.9 ± 1.1 | 3.470 ± 0.094 | 65.8 ± 1.0 | 1.2 ± 0.0 | 1.02 |
| `cat_global_large` | 71.56 ± 0.71 | 3.15 ± 0.11 | 66.84 ± 0.63 | 1.400 ± 0.021 | 1.04 |
| `cat_clang_large` | 72.4 ± 1.2 | 5.66 ± 0.18 | 66.0 ± 1.1 | 1.197 ± 0.021 | 1.05 |
| `cat_gforth_large` | 75.1 ± 1.2 | 7.09 ± 0.18 | 66.6 ± 1.0 | 2.134 ± 0.026 | 1.09 |
| `cat_go_large` | 83.2 ± 1.3 | 8.52 ± 0.25 | 74.3 ± 1.2 | 3.622 ± 0.055 | 1.21 |
| `cat_cl_sbcl_large` | 85.30 ± 0.61 | 9.58 ± 0.14 | 73.35 ± 0.63 | 39.709 ± 0.021 | 1.24 |
| `cat_python_large` | 91.30 ± 0.28 | 19.582 ± 0.099 | 69.51 ± 0.22 | 11.68 ± 0.11 | 1.33 |
| `cat_pypy_large` | 94.4 ± 1.8 | 16.81 ± 0.19 | 75.5 ± 1.6 | 69.20 ± 0.17 | 1.37 |
| `cat_astil_jit_large` | 96.83 ± 0.99 | 30.38 ± 0.23 | 65.77 ± 0.86 | 2.856 ± 0.026 | 1.41 |
| `cat_luajit_large` | 100.39 ± 0.98 | 29.98 ± 0.51 | 68.80 ± 0.44 | 1.928 ± 0.028 | 1.46 |
| `cat_js_bun_large` | 106.22 ± 0.43 | 52.13 ± 0.35 | 72.17 ± 0.35 | 73.594 ± 0.047 | 1.55 |
| `cat_java_large` | 114.97 ± 1.00 | 67.8 ± 1.3 | 92.96 ± 0.93 | 42.62 ± 0.56 | 1.67 |
| `cat_erlang_loop_large` | 4339.2 ± 15.0 | 4144.2 ± 17.8 | 134.5 ± 2.7 | 53.55 ± 0.29 | 63.20 |

## TCP CONNECTIONS

Measures 4096 concurrent connections with 32 one-byte request/echo exchanges per connection.

Wall time includes Python TCP driver work. After every connection closes, the driver kills and reaps the idle server; this work is also included in wall time. CPU time and peak mem/RSS measure only the server subprocess.

This benchmark is noisier than others, especially when using pthreads. Results vary more between reruns.

Results are sorted by total user+kernel CPU time.

Each implementation is measured 5 times.

| Command | Wall [ms] | User CPU [ms] | Kernel CPU [ms] | Peak mem [MiB] | CPU relative ↓ |
| --- | ---: | ---: | ---: | ---: | ---: |
| `tcp_conn_js_bun_callback` | 922.8 ± 9.2 | 93.23 ± 0.39 | 328.2 ± 2.7 | 30.73 ± 0.11 | 1.00 |
| `tcp_conn_luajit_luv_callback` | 906.1 ± 12.2 | 147.9 ± 2.1 | 277.3 ± 2.6 | 7.07 ± 0.28 | 1.01 |
| `tcp_conn_js_bun_coro` | 916.7 ± 19.3 | 155.2 ± 1.7 | 315.9 ± 3.1 | 58.05 ± 0.64 | 1.12 |
| `tcp_conn_luajit_luv_coro` | 914.3 ± 7.8 | 240.7 ± 1.9 | 291.7 ± 4.0 | 13.412 ± 0.046 | 1.26 |
| `tcp_conn_clang_pthread` | 1243.6 ± 55.7 | 59.93 ± 0.48 | 681.5 ± 6.7 | 65.2 ± 0.0 | 1.76 |
| `tcp_conn_astil_pthread_aot` | 1185.3 ± 123.5 | 60.82 ± 0.82 | 682.2 ± 3.8 | 65.819 ± 0.056 | 1.76 |
| `tcp_conn_zig_pthread` | 1320.6 ± 6.0 | 104.63 ± 0.56 | 691.7 ± 2.6 | 65.953 ± 0.025 | 1.89 |
| `tcp_conn_astil_pthread_jit` | 1234.7 ± 8.8 | 121.8 ± 1.4 | 678.8 ± 1.9 | 67.806 ± 0.092 | 1.90 |
| `tcp_conn_python_asyncio` | 1163.1 ± 41.8 | 641.9 ± 3.0 | 367.0 ± 11.4 | 44.69 ± 0.25 | 2.39 |
| `tcp_conn_cl_sbcl_thread` | 1253.7 ± 39.2 | 481.2 ± 3.2 | 1029.7 ± 181.6 | 758.0 ± 33.0 | 3.58 |
| `tcp_conn_java_thread` | 1167.8 ± 82.9 | 514.1 ± 27.2 | 1027.1 ± 84.5 | 547.4 ± 13.4 | 3.66 |
| `tcp_conn_pypy_asyncio` | 2007.7 ± 55.5 | 1242.7 ± 10.5 | 524.6 ± 7.3 | 116.7 ± 4.8 | 4.19 |
| `tcp_conn_go_goroutine` | 1217.5 ± 145.0 | 375.7 ± 2.5 | 2035.7 ± 24.2 | 18.575 ± 0.081 | 5.72 |
| `tcp_conn_erlang_active_n` | 1059.7 ± 14.5 | 892.1 ± 8.1 | 2503.2 ± 79.6 | 133.9 ± 2.0 | 8.06 |
| `tcp_conn_gforth_task` | 3761.2 ± 129.6 | 1589.2 ± 55.8 | 2117.1 ± 71.0 | 68.628 ± 0.036 | 8.79 |
| `tcp_conn_java_async` | 1029.3 ± 96.1 | 725.0 ± 12.7 | 3258.8 ± 435.9 | 80.66 ± 0.65 | 9.45 |
| `tcp_conn_java_vthread` | 1192.0 ± 101.7 | 984.6 ± 45.7 | 4718.4 ± 38.6 | 105.5 ± 1.5 | 13.53 |
| `tcp_conn_erlang_passive` | 1099.7 ± 41.3 | 1161.6 ± 12.6 | 6235.6 ± 50.1 | 115.9 ± 1.3 | 17.55 |
| `tcp_conn_python_thread` | 7139.7 ± 180.7 | 774.0 ± 5.4 | 47582.9 ± 1402.1 | 156.57 ± 0.11 | 114.74 |
