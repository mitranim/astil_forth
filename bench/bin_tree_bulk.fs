\ BOT-TRANSLATED from reg-CC file.

struct
  cell% field node-left
  cell% field node-right
end-struct node%

node% %size constant node-size

: node-tree-len ( depth -- len )
  2 swap lshift 1-
;

: node-tree-init { depth -- root }
  depth node-tree-len { len }
  len node-size * { size }
  size allocate throw { root }
  root size erase

  root { node }
  root node-size + { next }
  len 2/ 0 ?do
    next node node-left !
    next node-size + node node-right !
    node node-size + to node
    next node-size 2* + to next
  loop

  root
;

: node-tree-count ( node -- count )
  dup 0= if drop 0 exit then
  dup node-left @ recurse
  swap node-right @ recurse
  + 1+
;

1024 constant out-cap
create out out-cap allot
variable out-len

: out-reset ( -- )
  0 out-len !
;

: out+ ( addr len -- )
  dup out-len @ + out-cap > abort" output overflow"
  out out-len @ + swap dup out-len +! move
;

: out-u ( u -- )
  0 <# #s #> out+
;

: out-cr ( -- )
  s\" \n" out+
;

: report-tree { name name-len depth count -- }
  name name-len out+
  depth out-u
  s" ; count: " out+
  count out-u
  out-cr
;

: report-trees { iters depth count -- }
  iters out-u
  s"  trees of depth " out+
  depth out-u
  s" ; count: " out+
  count out-u
  out-cr
;

4 constant min-depth
18 constant max-depth
max-depth 1+ constant stretch-depth

: trees-at-depth { depth -- }
  0 { count }
  1 max-depth depth - min-depth + lshift { iters }

  iters 0 ?do
    depth node-tree-init
    dup node-tree-count count + to count
    free throw
  loop

  iters depth count report-trees
;

: verify-output ( -- )
  out out-len @
  s\" stretch tree of depth 19; count: 1048575\n262144 trees of depth 4; count: 8126464\n65536 trees of depth 6; count: 8323072\n16384 trees of depth 8; count: 8372224\n4096 trees of depth 10; count: 8384512\n1024 trees of depth 12; count: 8387584\n256 trees of depth 14; count: 8388352\n64 trees of depth 16; count: 8388544\n16 trees of depth 18; count: 8388592\nlong lived tree of depth 18; count: 524287\n"
  compare abort" wrong output"
;

: main ( -- )
  out-reset

  max-depth node-tree-init { long-lived-tree }

  stretch-depth node-tree-init { stretch-tree }
  s" stretch tree of depth "
  stretch-depth stretch-tree node-tree-count report-tree
  stretch-tree free throw

  max-depth 1+ min-depth do
    i trees-at-depth
  2 +loop

  s" long lived tree of depth "
  max-depth long-lived-tree node-tree-count report-tree
  long-lived-tree free throw

  verify-output
;

main
