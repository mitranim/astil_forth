import' std:lang.f

: fib { ind -- out }
  ind 1 <= if 1 ret end
  ind 1 - recur { prev }
  ind 2 - recur
  prev +
;

: main
  36 fib { val }

  \ " fib(36): " log val log_int lf
;
main
