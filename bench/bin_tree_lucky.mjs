// BOT-TRANSLATED from reg-CC file.

import { run } from "./bin_tree_run.mjs"

class Node {
  constructor(left, right) {
    this.left = left
    this.right = right
  }

  count() {
    const left = this.left
    if (left === null) return 1

    return 1 + left.count() + this.right.count()
  }

  static make(depth) {
    return depth
      ? new Node(this.make(depth - 1), this.make(depth - 1))
      : new Node(null, null)
  }
}

run(Node)
