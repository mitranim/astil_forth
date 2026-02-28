The interpreter makes it easy to disassemble words; under the hood, it shells out to `llvm-mc`, which must be globally available.

See `./1_disasm.f`.

```forth
: example_arith
  10 20 + 30 * { -- }
;
dis' example_arith
```

Result:

```asm
mov x0, #10
mov x1, #20
add x0, x0, x1
mov x1, #30
mul x0, x0, x1
ret
```

## Comparison with C

This amateur system is _parsecs_ behind optimizing C compilers. A full comparison is _way_ out of scope here. But here's a tiny example.

```forth
: word { one two three -- four }
  one two + three *
;
```

```c
long word(long one, long two, long three) {
  return (one + two) * three;
}
```

Our system:

```asm
add x0, x0, x1
mov x1, x2     // Avoidable with a better register allocator.
mul x0, x0, x1
ret
```

Clang with `-O1` or higher:

```asm
add x8, x1, x0
mul x0, x8, x2
ret
```
