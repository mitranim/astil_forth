// BOT-TRANSLATED from reg-CC file.

final class Node {
  final Node left;
  final Node right;

  Node(Node left, Node right) {
    this.left = left;
    this.right = right;
  }

  int count() {
    if (left == null) return 1;
    return 1 + left.count() + right.count();
  }

  static Node make(int depth) {
    if (depth == 0) return new Node(null, null);

    depth--;
    return new Node(make(depth), make(depth));
  }
}

final class bin_tree {
  public static void main(String[] args) {
    final var out = new StringBuilder();
    final int minDepth = 4;
    final int maxDepth = 18;
    final int stretchDepth = maxDepth + 1;
    final Node longLivedTree = Node.make(maxDepth);
    final Node stretchTree = Node.make(stretchDepth);

    out
      .append("stretch tree of depth ")
      .append(stretchDepth)
      .append("; count: ")
      .append(stretchTree.count())
      .append('\n');

    for (int depth = minDepth; depth <= maxDepth; depth += 2) {
      int count = 0;
      final int iters = 1 << (maxDepth - depth + minDepth);

      for (int run = 0; run < iters; run++) {
        final Node tree = Node.make(depth);
        count += tree.count();
      }

      out
        .append(iters)
        .append(" trees of depth ")
        .append(depth)
        .append("; count: ")
        .append(count)
        .append('\n');
    }

    out
      .append("long lived tree of depth ")
      .append(maxDepth)
      .append("; count: ")
      .append(longLivedTree.count())
      .append('\n');

    final String expected =
      "stretch tree of depth 19; count: 1048575\n"
        + "262144 trees of depth 4; count: 8126464\n"
        + "65536 trees of depth 6; count: 8323072\n"
        + "16384 trees of depth 8; count: 8372224\n"
        + "4096 trees of depth 10; count: 8384512\n"
        + "1024 trees of depth 12; count: 8387584\n"
        + "256 trees of depth 14; count: 8388352\n"
        + "64 trees of depth 16; count: 8388544\n"
        + "16 trees of depth 18; count: 8388592\n"
        + "long lived tree of depth 18; count: 524287\n";

    if (!out.toString().equals(expected)) throw new AssertionError(out);
  }
}
