\ BOT-ASSISTED

65536 constant CAP
512 constant RUNS
0xcbf29ce484222325 constant FNV_OFFSET
0x100000001b3 constant PRIME
0x8d8a704cbb222325 constant WANT
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
  dup
  s" FNV1A64_PRINT" getenv nip if ." 0x" hex dup 0 <# #s #> type decimal cr then
  WANT <> if 1 (bye) then
;

main
