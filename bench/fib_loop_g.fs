: fib ( src -- out )
  0 1 ( prev next )
  rot 0 ?do
    swap over +
  loop
  nip
;

: main
  1 16 lshift
  0
  ?do
    91 fib drop
  loop

  \ 91 fib . cr
;
main
