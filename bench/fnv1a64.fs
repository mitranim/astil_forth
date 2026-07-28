\ BOT-ASSISTED

65536 constant CAP
2048 constant RUNS
0xcbf29ce484222325 constant FNV_OFFSET
0x100000001b3 constant PRIME
0xb0a1ea8560222325 constant WANT
create buf CAP allot

: init
  s" 0123456789abcdef" { chunk len }
  buf CAP + buf ?do
    chunk i buf - len mod + c@ i c!
  loop
;

: fnv1a64 { src len -- hash }
  src len + src ?do
    i c@ xor PRIME *
  loop
;

: main
  init
  FNV_OFFSET
  RUNS 0 ?do
    buf CAP fnv1a64
  loop
  WANT <> if 1 (bye) then
;

main
