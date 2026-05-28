\ Slightly modified copy of Gforth's `sieve.fs`.

4096 constant RUNS
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
  \ find_prime .

  RUNS 0 do
    FLAGS LEN find_prime drop
  loop
;
main
