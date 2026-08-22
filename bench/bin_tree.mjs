// BOT-TRANSLATED from reg-CC file.

import {run} from "./bin_tree_run.mjs"

class Node {
  constructor(left, right) {
    this.left = left
    this.right = right
  }

  count() {
    return 1 + (this.left?.count() ?? 0) + (this.right?.count() ?? 0)
  }

  static make(depth) {
    return depth
      ? new Node(this.make(depth - 1), this.make(depth - 1))
      : new Node(null, null)
  }
}

run(Node)
