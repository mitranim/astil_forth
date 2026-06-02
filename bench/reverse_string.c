static void reverse(char *str, unsigned len) {
  for (auto low = str, high = (str + len - 1); low < high; low++, high--) {
    const auto tmp = *low;
    *low           = *high;
    *high          = tmp;
  }
}

static void *escape_ptr(void *val) {
  register void *x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0) : : "memory");
  return x0;
}

// #include <stdio.h>

int main() {
  char str[] = "0123456789abcdef";
  for (unsigned ind = 0; ind <= (1 << 22); ind++) {
    reverse(escape_ptr(str), sizeof(str) - 1);
  }
  // printf("str: %s\n", str);
}
