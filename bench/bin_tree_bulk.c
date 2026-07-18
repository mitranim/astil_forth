// BOT-TRANSLATED from reg-CC file.

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stddefer.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
  struct Node *left;
  struct Node *right;
} Node;

static size_t node_tree_len(int64_t depth) {
  return ((size_t)1 << (depth + 1)) - 1;
}

static Node *node_tree_init(int64_t depth) {
  const size_t len = node_tree_len(depth);
  Node *nodes      = calloc(len, sizeof(*nodes));
  if (!nodes) abort();

  for (size_t ind = 0; ind < len / 2; ind++) {
    Node *next       = &nodes[ind * 2 + 1];
    nodes[ind].left  = next;
    nodes[ind].right = next + 1;
  }

  return nodes;
}

static int64_t node_tree_count(const Node *node) {
  if (!node) return 0;
  return 1 + node_tree_count(node->left) + node_tree_count(node->right);
}

[[gnu::format(printf, 4, 5)]]
static void strf_into(
  char *buf,
  size_t cap,
  size_t *len,
  const char *format,
  ...
) {
  va_list args;
  va_start(args, format);
  const int wrote = vsnprintf(buf + *len, cap - *len, format, args);
  va_end(args);

  if (wrote < 0 || (size_t)wrote >= cap - *len) abort();
  *len += (size_t)wrote;
}

int main(void) {
  static const int64_t min_depth = 4;
  static const int64_t max_depth = 18;
  const int64_t stretch_depth    = max_depth + 1;

  char out[1024];
  size_t out_len = 0;

  Node *long_lived_tree = node_tree_init(max_depth);
  defer free(long_lived_tree);

  {
    Node *tree = node_tree_init(stretch_depth);
    defer free(tree);

    strf_into(
      out,
      sizeof(out),
      &out_len,
      "stretch tree of depth %" PRId64 "; count: %" PRId64 "\n",
      stretch_depth,
      node_tree_count(tree)
    );
  }

  for (int64_t depth = min_depth; depth <= max_depth; depth += 2) {
    int64_t count       = 0;
    const int64_t iters = INT64_C(1) << (max_depth - depth + min_depth);

    for (int64_t run = 0; run < iters; run++) {
      Node *tree = node_tree_init(depth);
      defer free(tree);
      count += node_tree_count(tree);
    }

    strf_into(
      out,
      sizeof(out),
      &out_len,
      "%" PRId64 " trees of depth %" PRId64 "; count: %" PRId64 "\n",
      iters,
      depth,
      count
    );
  }

  strf_into(
    out,
    sizeof(out),
    &out_len,
    "long lived tree of depth %" PRId64 "; count: %" PRId64 "\n",
    max_depth,
    node_tree_count(long_lived_tree)
  );

  static const char expected[] =
    "stretch tree of depth 19; count: 1048575\n"
    "262144 trees of depth 4; count: 8126464\n"
    "65536 trees of depth 6; count: 8323072\n"
    "16384 trees of depth 8; count: 8372224\n"
    "4096 trees of depth 10; count: 8384512\n"
    "1024 trees of depth 12; count: 8387584\n"
    "256 trees of depth 14; count: 8388352\n"
    "64 trees of depth 16; count: 8388544\n"
    "16 trees of depth 18; count: 8388592\n"
    "long lived tree of depth 18; count: 524287\n";

  if (strcmp(out, expected)) abort();
  return 0;
}
