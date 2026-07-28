\ BOT-TRANSLATED

: reverse-string { str len -- }
  len 2/ 0 ?do
    str i + c@
    str len 1- i - + c@
    str i + c!
    str len 1- i - + c!
  loop
;

create BUF 16 chars allot

: main
  s" 0123456789abcdef" BUF swap cmove
  1 25 lshift 0 ?do
    BUF 16 reverse-string
  loop
  BUF 16 reverse-string
  BUF 16 s" fedcba9876543210" compare
  abort" reverse-string output mismatch"
;
main
