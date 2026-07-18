// BOT-TRANSLATED from reg-CC file.

export function run(Node) {
  const minDepth = 4
  const maxDepth = 18
  const stretchDepth = maxDepth + 1

  let out = ``

  const longLivedTree = Node.make(maxDepth)

  const stretchTree = Node.make(stretchDepth)
  out += `stretch tree of depth ${stretchDepth}; count: ${stretchTree.count()}\n`

  for (let depth = minDepth; depth <= maxDepth; depth += 2) {
    let count = 0
    const iters = 1 << (maxDepth - depth + minDepth)

    for (let run = 0; run < iters; run++) {
      const tree = Node.make(depth)
      count += tree.count()
    }

    out += `${iters} trees of depth ${depth}; count: ${count}\n`
  }

  out += `long lived tree of depth ${maxDepth}; count: ${longLivedTree.count()}\n`

  const expected = `stretch tree of depth 19; count: 1048575
262144 trees of depth 4; count: 8126464
65536 trees of depth 6; count: 8323072
16384 trees of depth 8; count: 8372224
4096 trees of depth 10; count: 8384512
1024 trees of depth 12; count: 8387584
256 trees of depth 14; count: 8388352
64 trees of depth 16; count: 8388544
16 trees of depth 18; count: 8388592
long lived tree of depth 18; count: 524287
`

  if (out !== expected) throw Error(out)
}
