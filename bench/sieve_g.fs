\ Slightly modified copy of Gforth's `sieve.fs`.

16384 constant RUNS
8192 constant LEN
create FLAGS LEN allot

: find_prime { flags len -- num }
  flags len 1 fill
  0 3
  flags len + flags do
    i c@ if
      dup i +
      dup flags len + < if
        flags len + swap do 0 i c! dup +loop
      else
        drop
      then
      swap 1+ swap
    then
    2 +
  loop
  drop
;

: main
  0
  RUNS 0 do
    drop FLAGS LEN find_prime
  loop
  1899 <> abort" mismatch: expected 1899"
;
main
