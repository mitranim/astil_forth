: fib ( ind -- out )
  dup  1 <= if drop 1 exit then
  dup  1 - recurse
  swap 2 - recurse
  +
;

: main
  36 fib

  \ 36 fib . cr
  \ ." done" cr
;
main
