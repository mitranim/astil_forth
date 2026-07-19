: fib ( ind -- out )
  dup  1 <= if drop 1 exit then
  dup  1 - recurse
  swap 2 - recurse
  +
;

: main
  39 fib 102334155 <>
  abort" recursive Fibonacci output mismatch"
;
main
