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

In Erlang: `baseline_erlang_single`, CPU-oriented benchmarks, and "cat" use one BEAM scheduler (`+S 1:1`). TCP benchmark uses default scheduler count.

Notes:

- All measurements were done on M3 Pro.
- CPU microbenchmarks are sensitive to code layout and instruction selection. Small source changes can shift CPU frontend, cache, and branch-prediction behavior; on M3 Pro, we have seen cosmetic-looking changes move results by up to ≈10% or up to 10ms depending on benchmark runtime. Avoid over-generalizing differences in that range.
- The suite records wall time, total CPU time, and peak memory usage. GC-based engines can spend substantial CPU on background threads.
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

Erlang/OTP 28 [erts-16.4] [source] [64-bit] [smp:12:12] [ds:12:12:10] [async-threads:1] [jit] [dtrace]

[pypy 7.3.17 with gcc apple llvm 16.0.0 (clang-1600.0.26.4)]
python 3.10.14 (39dc8d3c85a7, nov 09 2024, 22:49:03)

python 3.14.4

Erlang/OTP 28 [erts-16.4] [source] [64-bit] [smp:12:12] [ds:12:12:10] [async-threads:1] [jit] [dtrace]
```

## NONE

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `none_astil_reg` | 1.176 ± 0.049 | 905.6 ± 47.6 | 1.4 ± 0.0 | 1.00 |
| `none_astil_stack` | 3.6 ± 1.4 | 2459.0 ± 653.7 | 1.353 ± 0.021 | 3.09 |

## BASELINE

| Command | Wall [ms] ↓ | CPU [µs] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `baseline_luajit` | 1.645 ± 0.048 | 866.4 ± 63.0 | 1.4 ± 0.0 | 1.00 |
| `baseline_gforth` | 2.912 ± 0.073 | 2012.0 ± 62.3 | 2.062 ± 0.011 | 1.77 |
| `baseline_astil_stack` | 3.151 ± 0.063 | 2834.8 ± 78.3 | 1.7 ± 0.0 | 1.92 |
| `baseline_js_bun` | 7.46 ± 0.30 | 6047.0 ± 264.0 | 19.209 ± 0.041 | 4.54 |
| `baseline_cl_sbcl` | 11.77 ± 0.18 | 10241.4 ± 138.2 | 39.2 ± 0.0 | 7.16 |
| `baseline_astil_reg` | 13.36 ± 0.74 | 12887.6 ± 724.1 | 2.3 ± 0.0 | 8.12 |
| `baseline_pypy` | 14.98 ± 0.23 | 13730.4 ± 291.5 | 27.984 ± 0.094 | 9.11 |
| `baseline_python` | 15.64 ± 0.29 | 14169.0 ± 138.3 | 11.43 ± 0.15 | 9.51 |
| `baseline_java` | 39.5 ± 4.1 | 40581.4 ± 1585.2 | 35.13 ± 0.21 | 24.01 |
| `baseline_erlang_default` | 72.3 ± 3.1 | 217679.8 ± 8238.4 | 58.26 ± 0.25 | 43.94 |
| `baseline_erlang_single` | 73.6 ± 5.1 | 71708.6 ± 1776.4 | 47.90 ± 0.34 | 44.73 |

## BUBBLE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bubble_clang` | 306.4 ± 3.2 | 305.6 ± 3.0 | 1.3 ± 0.0 | 1.00 |
| `bubble_js_bun` | 435.8 ± 1.5 | 438.1 ± 1.6 | 28.631 ± 0.076 | 1.42 |
| `bubble_astil_aot` | 495.0 ± 2.4 | 494.1 ± 2.2 | 1.334 ± 0.014 | 1.62 |
| `bubble_cl_sbcl` | 501.0 ± 1.0 | 498.7 ± 1.0 | 40.3 ± 0.0 | 1.64 |
| `bubble_astil_jit` | 509.5 ± 6.6 | 508.6 ± 6.3 | 2.64 ± 0.16 | 1.66 |
| `bubble_java` | 532.6 ± 2.8 | 536.7 ± 2.9 | 35.74 ± 0.21 | 1.74 |
| `bubble_luajit` | 574.8 ± 14.1 | 573.8 ± 14.1 | 1.981 ± 0.017 | 1.88 |
| `bubble_pypy` | 1136.2 ± 3.0 | 1134.6 ± 3.0 | 31.381 ± 0.065 | 3.71 |
| `bubble_astil_stack` | 1590.8 ± 11.8 | 1588.9 ± 11.4 | 2.0 ± 0.0 | 5.19 |
| `bubble_gforth` | 3781.7 ± 138.9 | 3780.1 ± 138.9 | 2.350 ± 0.045 | 12.34 |
| `bubble_erlang_atomics` | 9044.4 ± 91.9 | 7585.7 ± 93.3 | 48.17 ± 0.27 | 29.52 |
| `bubble_python` | 26972.8 ± 186.0 | 26970.6 ± 186.0 | 12.994 ± 0.021 | 88.04 |

## PRIME SIEVE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `sieve_clang` | 152.52 ± 0.38 | 151.94 ± 0.40 | 1.2 ± 0.0 | 1.00 |
| `sieve_cl_sbcl` | 183.0 ± 5.2 | 180.7 ± 4.9 | 40.2 ± 0.0 | 1.20 |
| `sieve_js_bun` | 216.7 ± 8.2 | 216.6 ± 8.2 | 27.356 ± 0.062 | 1.42 |
| `sieve_astil_aot` | 219.2 ± 3.1 | 218.6 ± 3.1 | 1.2 ± 0.0 | 1.44 |
| `sieve_astil_jit` | 230.8 ± 2.3 | 230.2 ± 2.3 | 2.350 ± 0.049 | 1.51 |
| `sieve_luajit` | 247.44 ± 0.42 | 245.59 ± 0.32 | 1.6 ± 0.0 | 1.62 |
| `sieve_java` | 250.1 ± 2.3 | 258.2 ± 3.4 | 35.2 ± 2.0 | 1.64 |
| `sieve_pypy` | 582.8 ± 2.9 | 581.2 ± 3.0 | 31.709 ± 0.051 | 3.82 |
| `sieve_astil_stack` | 704.4 ± 2.5 | 703.7 ± 2.4 | 1.7 ± 0.0 | 4.62 |
| `sieve_gforth` | 1757.1 ± 25.6 | 1755.7 ± 25.5 | 2.084 ± 0.009 | 11.52 |
| `sieve_erlang_atomics` | 3667.3 ± 28.4 | 2924.0 ± 27.9 | 47.92 ± 0.17 | 24.04 |
| `sieve_python` | 15320.4 ± 14.8 | 15316.0 ± 14.6 | 11.74 ± 0.11 | 100.45 |

## REVERSE STRING

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `reverse_string_clang` | 90.26 ± 0.19 | 89.74 ± 0.21 | 1.169 ± 0.017 | 1.00 |
| `reverse_string_java` | 137.75 ± 0.74 | 145.3 ± 1.1 | 36.33 ± 0.35 | 1.53 |
| `reverse_string_js_bun` | 218.77 ± 0.14 | 222.581 ± 0.079 | 30.909 ± 0.043 | 2.42 |
| `reverse_string_astil_aot` | 232.0 ± 3.1 | 231.4 ± 3.1 | 1.206 ± 0.017 | 2.57 |
| `reverse_string_astil_jit` | 243.5 ± 1.8 | 242.8 ± 1.8 | 2.356 ± 0.026 | 2.70 |
| `reverse_string_luajit` | 323.9 ± 1.9 | 322.2 ± 1.8 | 1.7 ± 0.0 | 3.59 |
| `reverse_string_cl_sbcl` | 709.1 ± 29.8 | 706.6 ± 29.6 | 39.9 ± 0.0 | 7.86 |
| `reverse_string_pypy` | 912.9 ± 4.5 | 911.1 ± 4.5 | 32.24 ± 0.14 | 10.11 |
| `reverse_string_erlang_list` | 1090.5 ± 4.6 | 942.3 ± 5.2 | 47.65 ± 0.16 | 12.08 |
| `reverse_string_astil_stack` | 1767.2 ± 25.5 | 1766.4 ± 25.5 | 1.744 ± 0.021 | 19.58 |
| `reverse_string_erlang_binary` | 3252.1 ± 15.4 | 2781.4 ± 15.8 | 47.71 ± 0.30 | 36.03 |
| `reverse_string_gforth` | 4173.9 ± 3.2 | 4172.4 ± 3.3 | 2.072 ± 0.028 | 46.24 |
| `reverse_string_python` | 13156.4 ± 131.1 | 13153.9 ± 131.2 | 11.691 ± 0.043 | 145.75 |

## FIB_LOOP

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_astil_asm_aot` | 102.36 ± 0.26 | 101.86 ± 0.21 | 1.2 ± 0.0 | 1.00 |
| `fib_loop_clang` | 118.2 ± 2.2 | 117.7 ± 2.2 | 1.147 ± 0.014 | 1.15 |
| `fib_loop_java` | 144.1 ± 3.5 | 149.0 ± 3.7 | 35.337 ± 0.050 | 1.41 |
| `fib_loop_astil_aot` | 190.378 ± 0.059 | 189.769 ± 0.034 | 1.2 ± 0.0 | 1.86 |
| `fib_loop_cl_sbcl` | 192.9 ± 2.3 | 190.2 ± 2.5 | 92.366 ± 0.009 | 1.88 |
| `fib_loop_astil_jit` | 204.5 ± 5.4 | 203.9 ± 5.4 | 2.337 ± 0.021 | 2.00 |
| `fib_loop_js_bun` | 272.97 ± 0.88 | 274.4 ± 1.3 | 27.89 ± 0.16 | 2.67 |
| `fib_loop_luajit` | 371.1 ± 3.3 | 369.5 ± 3.2 | 1.6 ± 0.0 | 3.63 |
| `fib_loop_pypy` | 802.4 ± 4.0 | 800.6 ± 4.2 | 31.48 ± 0.66 | 7.84 |
| `fib_loop_astil_stack` | 1144.5 ± 42.6 | 1143.9 ± 42.5 | 1.744 ± 0.021 | 11.18 |
| `fib_loop_erlang` | 1180.9 ± 8.3 | 816.7 ± 10.6 | 47.64 ± 0.27 | 11.54 |
| `fib_loop_gforth` | 2258.6 ± 121.4 | 2257.1 ± 121.4 | 2.094 ± 0.040 | 22.06 |
| `fib_loop_python` | 12766.1 ± 53.8 | 12763.3 ± 53.7 | 11.641 ± 0.061 | 124.72 |

## FIB_LOOP_BIG

C and Astil use `uint128`. Other languages use actual bigints.

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_loop_big_clang` | 112.23 ± 0.26 | 111.66 ± 0.23 | 1.1 ± 0.0 | 1.00 |
| `fib_loop_big_astil_asm_aot` | 135.6 ± 4.0 | 135.0 ± 4.1 | 1.163 ± 0.014 | 1.21 |
| `fib_loop_big_astil_asm_jit` | 142.9 ± 5.5 | 142.4 ± 5.5 | 2.372 ± 0.026 | 1.27 |
| `fib_loop_big_astil_aot` | 268.6 ± 1.7 | 267.9 ± 1.6 | 1.2 ± 0.0 | 2.39 |
| `fib_loop_big_astil_jit` | 270.0 ± 9.0 | 269.3 ± 9.0 | 2.347 ± 0.026 | 2.41 |
| `fib_loop_big_cl_sbcl` | 1983.1 ± 5.5 | 1982.6 ± 5.4 | 96.731 ± 0.094 | 17.67 |
| `fib_loop_big_java` | 2465.9 ± 13.2 | 2543.7 ± 14.1 | 504.41 ± 0.26 | 21.97 |
| `fib_loop_big_pypy` | 2734.2 ± 6.4 | 2732.3 ± 6.4 | 35.941 ± 0.079 | 24.36 |
| `fib_loop_big_erlang` | 2767.9 ± 11.7 | 2415.5 ± 13.5 | 48.08 ± 0.49 | 24.66 |
| `fib_loop_big_js_bun` | 3230.9 ± 11.7 | 3250.9 ± 10.2 | 61.053 ± 0.055 | 28.79 |
| `fib_loop_big_python` | 13459.9 ± 14.2 | 13457.3 ± 14.2 | 11.697 ± 0.062 | 119.93 |

## FIB_RECURSIVE: fib(39)

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fib_rec_clang` | 149.6 ± 1.8 | 149.0 ± 1.8 | 1.1 ± 0.0 | 1.00 |
| `fib_rec_java` | 197.5 ± 1.3 | 200.7 ± 1.5 | 35.269 ± 0.095 | 1.32 |
| `fib_rec_astil_aot` | 236.3 ± 2.9 | 235.6 ± 2.9 | 1.169 ± 0.017 | 1.58 |
| `fib_rec_astil_jit` | 247.0 ± 6.4 | 246.3 ± 6.4 | 2.3 ± 0.0 | 1.65 |
| `fib_rec_js_bun` | 259.7 ± 3.4 | 258.8 ± 3.3 | 26.137 ± 0.069 | 1.74 |
| `fib_rec_luajit` | 273.1 ± 3.5 | 271.4 ± 3.3 | 1.659 ± 0.007 | 1.83 |
| `fib_rec_cl_sbcl` | 323.6 ± 10.1 | 321.1 ± 10.1 | 38.972 ± 0.028 | 2.16 |
| `fib_rec_pypy` | 466.3 ± 2.6 | 464.8 ± 2.6 | 46.61 ± 0.15 | 3.12 |
| `fib_rec_erlang` | 857.4 ± 3.1 | 404.9 ± 5.1 | 47.62 ± 0.27 | 5.73 |
| `fib_rec_astil_stack` | 1053.5 ± 29.4 | 1052.9 ± 29.4 | 1.8 ± 0.0 | 7.04 |
| `fib_rec_gforth` | 1430.5 ± 9.0 | 1429.2 ± 9.0 | 2.062 ± 0.027 | 9.56 |
| `fib_rec_python` | 6455.9 ± 38.7 | 6452.7 ± 39.1 | 11.67 ± 0.13 | 43.15 |

## CONST FOLD

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `const_fold_folded_astil_aot` | 137.9 ± 2.8 | 137.3 ± 2.9 | 1.163 ± 0.014 | 1.00 |
| `const_fold_folded_astil_jit` | 146.9 ± 2.2 | 146.3 ± 2.1 | 2.3 ± 0.0 | 1.07 |
| `const_fold_runtime_astil_aot` | 2711.0 ± 4.3 | 2710.3 ± 4.2 | 1.163 ± 0.014 | 19.66 |
| `const_fold_runtime_astil_jit` | 2723.3 ± 3.3 | 2722.5 ± 3.3 | 2.331 ± 0.007 | 19.75 |

## FNV-1A 64

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `fnv1a64_clang` | 136.0 ± 3.1 | 135.4 ± 3.1 | 1.2 ± 0.0 | 1.00 |
| `fnv1a64_astil_aot` | 137.7 ± 4.3 | 137.1 ± 4.3 | 1.2 ± 0.0 | 1.01 |
| `fnv1a64_cl_sbcl` | 148.5 ± 1.1 | 145.86 ± 0.89 | 39.678 ± 0.024 | 1.09 |
| `fnv1a64_astil_jit` | 154.99 ± 0.59 | 154.44 ± 0.57 | 2.331 ± 0.007 | 1.14 |
| `fnv1a64_java` | 177.7 ± 2.2 | 184.8 ± 1.7 | 35.99 ± 0.13 | 1.31 |
| `fnv1a64_pypy` | 277.8 ± 2.2 | 276.0 ± 2.3 | 31.231 ± 0.056 | 2.04 |
| `fnv1a64_gforth` | 524.5 ± 10.6 | 523.3 ± 10.6 | 2.119 ± 0.009 | 3.86 |
| `fnv1a64_astil_stack` | 661.9 ± 101.3 | 661.2 ± 101.3 | 1.8 ± 0.0 | 4.87 |
| `fnv1a64_luajit` | 1102.9 ± 3.1 | 1101.4 ± 3.0 | 1.803 ± 0.014 | 8.11 |
| `fnv1a64_js_bun` | 1544.2 ± 19.4 | 1546.4 ± 19.6 | 29.106 ± 0.079 | 11.36 |
| `fnv1a64_erlang` | 4287.9 ± 42.8 | 4167.3 ± 42.9 | 47.72 ± 0.51 | 31.54 |
| `fnv1a64_python` | 9827.9 ± 92.6 | 9825.5 ± 92.5 | 11.77 ± 0.15 | 72.29 |

## SCAN DELIMS

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `scan_delims_c_simd` | 143.86 ± 0.14 | 143.28 ± 0.14 | 1.2 ± 0.0 | 1.00 |
| `scan_delims_astil_simd_aot` | 144.68 ± 0.76 | 144.05 ± 0.81 | 1.2 ± 0.0 | 1.01 |
| `scan_delims_astil_simd_jit` | 157.6 ± 1.2 | 157.0 ± 1.2 | 2.4 ± 0.0 | 1.10 |
| `scan_delims_java_simd` | 307.0 ± 7.3 | 405.4 ± 8.1 | 178.57 ± 0.78 | 2.13 |
| `scan_delims_c_naive` | 398.1 ± 1.8 | 397.4 ± 1.8 | 1.2 ± 0.0 | 2.77 |
| `scan_delims_astil_cell_aot` | 573.8 ± 1.7 | 573.0 ± 1.8 | 1.2 ± 0.0 | 3.99 |
| `scan_delims_astil_cell_jit` | 585.6 ± 2.2 | 584.9 ± 2.2 | 2.347 ± 0.007 | 4.07 |
| `scan_delims_luajit` | 667.5 ± 1.8 | 665.8 ± 1.9 | 1.741 ± 0.014 | 4.64 |
| `scan_delims_java` | 674.3 ± 4.4 | 683.0 ± 4.2 | 36.12 ± 0.17 | 4.69 |
| `scan_delims_js_bun` | 931.4 ± 4.2 | 935.6 ± 4.2 | 30.67 ± 0.11 | 6.47 |
| `scan_delims_cl_sbcl` | 1132.1 ± 8.7 | 1129.3 ± 8.6 | 40.341 ± 0.020 | 7.87 |
| `scan_delims_astil_naive_aot` | 1137.2 ± 2.1 | 1136.5 ± 2.1 | 1.2 ± 0.0 | 7.91 |
| `scan_delims_astil_naive_jit` | 1149.9 ± 3.1 | 1149.0 ± 3.1 | 2.331 ± 0.007 | 7.99 |
| `scan_delims_pypy` | 1414.4 ± 2.6 | 1412.5 ± 2.6 | 31.194 ± 0.099 | 9.83 |
| `scan_delims_erlang_naive` | 2154.9 ± 2.7 | 1912.6 ± 2.3 | 47.54 ± 0.40 | 14.98 |
| `scan_delims_python` | 4940.0 ± 86.9 | 4937.2 ± 87.1 | 11.63 ± 0.13 | 34.34 |

## BINARY TREE

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_java` | 296.1 ± 1.6 | 362.8 ± 10.5 | 912.1 ± 22.4 | 1.00 |
| `bin_tree_cl_sbcl` | 299.18 ± 0.76 | 296.17 ± 0.68 | 112.522 ± 0.014 | 1.01 |
| `bin_tree_astil_aot` | 325.5 ± 14.9 | 324.2 ± 14.9 | 25.2 ± 0.0 | 1.10 |
| `bin_tree_astil_jit` | 327.7 ± 3.0 | 326.9 ± 2.9 | 26.366 ± 0.021 | 1.11 |
| `bin_tree_js_bun` | 405.3 ± 4.7 | 596.8 ± 14.1 | 192.2 ± 7.4 | 1.05 |
| `bin_tree_zig` | 418.0 ± 2.1 | 417.3 ± 2.1 | 25.9 ± 0.0 | 1.41 |
| `bin_tree_clang` | 1053.6 ± 11.1 | 1052.8 ± 11.1 | 27.1 ± 0.0 | 3.56 |
| `bin_tree_erlang` | 1077.9 ± 3.7 | 599.1 ± 4.0 | 177.75 ± 0.16 | 3.64 |
| `bin_tree_go` | 1119.2 ± 3.8 | 4524.8 ± 9.9 | 45.3 ± 1.9 | 3.78 |
| `bin_tree_luajit` | 2028.6 ± 9.6 | 2027.1 ± 9.5 | 278.7 ± 25.6 | 6.85 |
| `bin_tree_astil_stack` | 2205.3 ± 59.1 | 2204.6 ± 59.1 | 25.759 ± 0.021 | 7.45 |
| `bin_tree_pypy` | 2620.8 ± 22.4 | 2617.9 ± 22.5 | 180.0 ± 35.4 | 8.85 |
| `bin_tree_gforth` | 4725.5 ± 18.2 | 4724.0 ± 18.2 | 28.056 ± 0.009 | 15.96 |
| `bin_tree_python` | 8225.3 ± 15.0 | 8222.6 ± 15.2 | 84.100 ± 0.014 | 27.77 |

## BINARY TREE BULK

| Command | Wall [ms] ↓ | CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: |
| `bin_tree_clang_bulk` | 151.3 ± 6.4 | 150.6 ± 6.3 | 35.9 ± 0.0 | 1.00 |
| `bin_tree_astil_bulk_aot` | 197.3 ± 1.5 | 196.4 ± 1.5 | 25.209 ± 0.014 | 1.30 |
| `bin_tree_astil_bulk_jit` | 210.0 ± 2.2 | 209.4 ± 2.2 | 26.337 ± 0.021 | 1.39 |
| `bin_tree_go_bulk` | 373.0 ± 9.9 | 729.5 ± 11.0 | 57.7 ± 3.4 | 2.46 |
| `bin_tree_astil_stack_bulk` | 1613.5 ± 22.2 | 1612.8 ± 22.2 | 25.8 ± 0.0 | 10.66 |
| `bin_tree_gforth_bulk` | 1633.1 ± 16.8 | 1631.7 ± 16.7 | 38.4 ± 3.6 | 10.79 |
| `bin_tree_bulk_erlang_binary` | 1839.7 ± 13.6 | 1431.2 ± 13.3 | 136.90 ± 0.38 | 12.16 |
| `bin_tree_bulk_erlang_atomics` | 2918.8 ± 8.2 | 1883.2 ± 9.5 | 91.87 ± 0.38 | 19.29 |

## CAT

Copies a warmed 512 MiB file and 512 MiB stdin (1 GiB total).

| Command | Wall [ms] ↓ | User CPU [ms] | Kernel CPU [ms] | Peak mem [MiB] | Relative |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cat_zig` | 63.77 ± 0.71 | 2.514 ± 0.082 | 60.54 ± 0.64 | 1.941 ± 0.026 | 1.00 |
| `cat_astil_aot` | 67.18 ± 0.71 | 3.593 ± 0.058 | 62.90 ± 0.65 | 1.2 ± 0.0 | 1.05 |
| `cat_global` | 69.0 ± 1.2 | 3.205 ± 0.059 | 64.2 ± 1.1 | 1.400 ± 0.021 | 1.08 |
| `cat_clang` | 69.5 ± 1.0 | 5.51 ± 0.17 | 63.32 ± 0.91 | 1.216 ± 0.026 | 1.09 |
| `cat_gforth` | 71.73 ± 0.87 | 6.97 ± 0.25 | 63.38 ± 0.69 | 2.156 ± 0.029 | 1.12 |
| `cat_go` | 80.4 ± 1.7 | 8.60 ± 0.24 | 71.5 ± 1.5 | 3.597 ± 0.050 | 1.26 |
| `cat_cl_sbcl` | 82.7 ± 1.5 | 9.76 ± 0.27 | 70.4 ± 1.2 | 39.7 ± 0.0 | 1.30 |
| `cat_python` | 86.44 ± 0.65 | 19.70 ± 0.15 | 64.48 ± 0.66 | 11.606 ± 0.098 | 1.36 |
| `cat_astil_jit` | 91.7 ± 1.3 | 28.62 ± 0.33 | 62.4 ± 1.0 | 2.822 ± 0.021 | 1.44 |
| `cat_luajit` | 93.69 ± 0.69 | 29.52 ± 0.45 | 62.55 ± 0.35 | 1.916 ± 0.014 | 1.47 |
| `cat_pypy` | 96.2 ± 1.4 | 16.67 ± 0.16 | 77.3 ± 1.4 | 69.32 ± 0.25 | 1.51 |
| `cat_js_bun` | 101.76 ± 0.43 | 52.68 ± 0.30 | 67.52 ± 0.46 | 73.65 ± 0.14 | 1.60 |
| `cat_java` | 112.4 ± 1.1 | 66.67 ± 0.50 | 91.4 ± 1.1 | 42.87 ± 0.76 | 1.76 |
| `cat_erlang_loop` | 139.7 ± 1.6 | 69.67 ± 0.32 | 100.6 ± 1.1 | 50.77 ± 0.34 | 2.19 |

## TCP CONNECTIONS

Measures 4096 concurrent connections with 32 one-byte request/echo exchanges per connection.

Wall time includes Python TCP driver work. After every connection closes, the driver kills and reaps the idle server; this work is also included in wall time. CPU time and peak mem/RSS measure only the server subprocess.

Erlang uses `+sbwt none`; it dramatically reduces active-N server CPU without changing the default scheduler count.

This benchmark is noisier than others, especially when using pthreads. Results vary more between reruns.

Results are sorted by total user+kernel CPU time.

Each implementation is measured 5 times.

| Command | Wall [ms] | User CPU [ms] | Kernel CPU [ms] | Peak mem [MiB] | CPU relative ↓ |
| --- | ---: | ---: | ---: | ---: | ---: |
| `tcp_conn_js_bun_callback` | 884.2 ± 24.0 | 92.08 ± 0.55 | 333.6 ± 6.5 | 30.58 ± 0.14 | 1.00 |
| `tcp_conn_luajit_luv_callback` | 860.1 ± 11.4 | 155.5 ± 1.3 | 274.3 ± 2.0 | 6.900 ± 0.024 | 1.01 |
| `tcp_conn_js_bun_coro` | 869.2 ± 21.3 | 159.1 ± 1.5 | 325.1 ± 7.3 | 59.50 ± 0.43 | 1.14 |
| `tcp_conn_luajit_luv_coro` | 837.6 ± 30.6 | 241.4 ± 1.9 | 279.8 ± 8.0 | 13.42 ± 0.10 | 1.22 |
| `tcp_conn_clang_pthread` | 1247.5 ± 55.0 | 68.4 ± 1.0 | 712.2 ± 10.4 | 65.2 ± 0.0 | 1.83 |
| `tcp_conn_astil_pthread_aot` | 1265.0 ± 20.9 | 70.68 ± 0.82 | 714.2 ± 10.2 | 65.866 ± 0.049 | 1.84 |
| `tcp_conn_zig_pthread` | 1313.3 ± 15.1 | 106.67 ± 0.77 | 712.7 ± 4.5 | 65.991 ± 0.038 | 1.92 |
| `tcp_conn_astil_pthread_jit` | 1170.4 ± 97.1 | 127.7 ± 6.0 | 700.4 ± 9.4 | 67.750 ± 0.095 | 1.95 |
| `tcp_conn_python_asyncio` | 1108.0 ± 18.4 | 631.3 ± 2.6 | 368.6 ± 7.3 | 44.23 ± 0.52 | 2.35 |
| `tcp_conn_java_thread` | 1144.0 ± 106.8 | 516.4 ± 32.0 | 1040.3 ± 179.5 | 533.9 ± 35.9 | 3.66 |
| `tcp_conn_cl_sbcl_thread` | 1417.1 ± 206.6 | 494.3 ± 33.6 | 1156.0 ± 255.4 | 884.9 ± 250.5 | 3.88 |
| `tcp_conn_pypy_asyncio` | 2143.6 ± 144.5 | 1253.8 ± 20.2 | 577.3 ± 12.4 | 124.9 ± 7.6 | 4.30 |
| `tcp_conn_go_goroutine` | 1123.9 ± 102.6 | 367.6 ± 2.8 | 2285.0 ± 104.9 | 18.538 ± 0.056 | 6.23 |
| `tcp_conn_erlang_active_n` | 995.7 ± 25.0 | 882.7 ± 19.0 | 2470.2 ± 127.7 | 133.8 ± 2.5 | 7.88 |
| `tcp_conn_gforth_task` | 3684.0 ± 108.8 | 1584.4 ± 64.2 | 2012.0 ± 48.7 | 68.662 ± 0.024 | 8.45 |
| `tcp_conn_java_async` | 1018.9 ± 158.1 | 754.9 ± 36.9 | 2932.6 ± 128.4 | 86.6 ± 3.6 | 8.66 |
| `tcp_conn_java_vthread` | 1079.7 ± 148.9 | 1003.5 ± 17.1 | 3717.6 ± 493.1 | 105.7 ± 3.4 | 11.09 |
| `tcp_conn_erlang_passive` | 1023.9 ± 26.2 | 1121.5 ± 13.3 | 5721.6 ± 258.4 | 115.1 ± 1.8 | 16.07 |
| `tcp_conn_python_thread` | 6800.0 ± 229.8 | 777.2 ± 7.2 | 44370.7 ± 2388.2 | 156.55 ± 0.14 | 106.05 |
