// BOT-TRANSLATED from reg-CC file.

import {run} from "./bin_tree_run.mjs"

class Node {
  constructor(left, right) {
    this.left = left
    this.right = right
  }

  count() {
    // If added: ≈2.5 times faster.
    // if (!this.left) return 1

    return 1 + (this.left && this.left.count()) + (this.right && this.right.count())
  }

  static make(depth) {
    return depth
      ? new Node(this.make(depth - 1), this.make(depth - 1))
      : new Node(null, null)
  }
}

run(Node)
