#include "./util.c"
#include <stdlib.h>
#include <string.h>

static void reverse(char *str, unsigned len) {
  for (auto low = str, high = (str + len - 1); low < high; low++, high--) {
    const auto tmp = *low;
    *low           = *high;
    *high          = tmp;
  }
}

// #include <stdio.h>

int main() {
  char        str[] = "0123456789abcdef";
  char *const input = escape_ptr(str);
  const auto  len   = (unsigned)escape_u64(sizeof(str) - 1);
  const auto  runs  = (unsigned)escape_u64(1 << 25);

  for (unsigned ind = 0; ind < runs; ind++) {
    reverse(input, len);
  }
  reverse(input, len);
  if (strcmp(str, "fedcba9876543210")) abort();
}
