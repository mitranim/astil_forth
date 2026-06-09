: fib ( depth -- out )
  0 1 ( prev next )
  rot 0 ?do swap over + loop
  nip
;

: main
  1 20 lshift \ runs
  0 ?do 91 fib drop loop
  \ 91 fib . cr
;
main
