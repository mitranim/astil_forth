// BOT-TRANSLATED from reg-CC file.

package main

import (
	"fmt"
	"strings"
)

type Node struct {
	left  *Node
	right *Node
}

func nodeTreeInit(depth int) *Node {
	node := &Node{}

	if depth != 0 {
		node.left = nodeTreeInit(depth - 1)
		node.right = nodeTreeInit(depth - 1)
	}

	return node
}

func (self *Node) count() int {
	if self == nil {
		return 0
	}
	return 1 + self.left.count() + self.right.count()
}

func main() {
	const minDepth = 4
	const maxDepth = 18
	const stretchDepth = maxDepth + 1

	var out strings.Builder

	longLivedTree := nodeTreeInit(maxDepth)

	tree := nodeTreeInit(stretchDepth)
	fmt.Fprintf(
		&out,
		"stretch tree of depth %d; count: %d\n",
		stretchDepth,
		tree.count(),
	)

	for depth := minDepth; depth <= maxDepth; depth += 2 {
		count := 0
		iters := 1 << (maxDepth - depth + minDepth)

		for run := 0; run < iters; run++ {
			tree := nodeTreeInit(depth)
			count += tree.count()
		}

		fmt.Fprintf(
			&out,
			"%d trees of depth %d; count: %d\n",
			iters,
			depth,
			count,
		)
	}

	fmt.Fprintf(
		&out,
		"long lived tree of depth %d; count: %d\n",
		maxDepth,
		longLivedTree.count(),
	)

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

	if out.String() != expected {
		panic(out.String())
	}
}
