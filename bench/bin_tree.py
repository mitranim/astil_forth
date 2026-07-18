# BOT-TRANSLATED from reg-CC file.

import gc
import sys


class Node:
    __slots__ = ("left", "right")

    def __init__(self, left=None, right=None):
        self.left = left
        self.right = right

    def count(self):
        if self.left is None:
            return 1
        return 1 + self.left.count() + self.right.count()


def node_tree_init(depth):
    if depth == 0:
        return Node()

    depth -= 1
    return Node(node_tree_init(depth), node_tree_init(depth))


def main():
    # Trees cannot contain cycles; CPython's reference counting handles their lifetime.
    if sys.implementation.name == "cpython":
        gc.disable()

    min_depth = 4
    max_depth = 18
    stretch_depth = max_depth + 1

    out = []
    long_lived_tree = node_tree_init(max_depth)

    tree = node_tree_init(stretch_depth)
    out.append(f"stretch tree of depth {stretch_depth}; count: {tree.count()}\n")
    del tree

    for depth in range(min_depth, max_depth + 1, 2):
        count = 0
        iters = 1 << (max_depth - depth + min_depth)

        for _ in range(iters):
            tree = node_tree_init(depth)
            count += tree.count()
            del tree

        out.append(f"{iters} trees of depth {depth}; count: {count}\n")

    out.append(
        f"long lived tree of depth {max_depth}; count: {long_lived_tree.count()}\n"
    )

    expected = (
        "stretch tree of depth 19; count: 1048575\n"
        "262144 trees of depth 4; count: 8126464\n"
        "65536 trees of depth 6; count: 8323072\n"
        "16384 trees of depth 8; count: 8372224\n"
        "4096 trees of depth 10; count: 8384512\n"
        "1024 trees of depth 12; count: 8387584\n"
        "256 trees of depth 14; count: 8388352\n"
        "64 trees of depth 16; count: 8388544\n"
        "16 trees of depth 18; count: 8388592\n"
        "long lived tree of depth 18; count: 524287\n"
    )

    actual = "".join(out)
    if actual != expected:
        raise AssertionError(actual)


if __name__ == "__main__":
    main()
