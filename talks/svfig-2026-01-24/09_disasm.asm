/*
## Source code

  : mock
    10 20 + 30 / 40 mod
  ;
*/

// ## Stack-based call convention

mov  x8, #10
str  x8, [x27], #8
mov  x8, #20
str  x8, [x27], #8
ldp  x1, x2, [x27, #-16]!
add  x1, x1, x2
str  x1, [x27], #8
mov  x8, #30
str  x8, [x27], #8
ldp  x1, x2, [x27, #-16]!
sdiv x1, x1, x2
str  x1, [x27], #8
mov  x8, #40
str  x8, [x27], #8
ldp  x1, x2, [x27, #-16]!
sdiv x3, x1, x2
msub x1, x3, x2, x1
str  x1, [x27], #8
ret

// ## Register-based call convention

mov  x0, #10
mov  x1, #20
add  x0, x0, x1
mov  x1, #30
sdiv x0, x0, x0
mov  x1, #40
sdiv x2, x0, x0
msub x0, x2, x1, x0
ret
