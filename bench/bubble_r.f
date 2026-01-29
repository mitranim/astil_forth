import' ../forth/lang_r.f

\ Translated from Gforth's `bubble.fs`.

74755            var:   SEED
8192             let:   LEN
LEN              cells: LIST
LEN cells LIST + let:   CEIL

: pseudo_random { -- val }
  SEED @ 1309 * 13849 + 65535 and { val }
  val SEED !
  val
;

: list_init
  CEIL LIST cell
  +loop: ptr
    pseudo_random ptr !
  #end
;

: list_dump
  CEIL LIST cell
  +loop: ptr
    ptr @ .
  #end
;

: list_verify
  CEIL -cell
  LIST cell
  +loop: ptr
    ptr @2 > throw_if" bubble_sort: not sorted"
  #end
;

: bubble
  CEIL LIST cell
  -loop: ceil_ptr
    ceil_ptr LIST cell
    +loop: ptr
      ptr @2 > #if
        ptr @2 { one two }
        two one ptr !2
      #end
    #end
  #end
;

: bubble_sort
  list_init
  bubble
  \ list_dump
  list_verify
;

: main
  bubble_sort
  log" [bubble] done" lf
;
main
