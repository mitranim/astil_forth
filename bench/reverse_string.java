// BOT-TRANSLATED

final class reverse_string {
  static volatile int runs = 1 << 25;

  static void reverse(char[] str) {
    for (int low = 0, high = str.length - 1; low < high; low++, high--) {
      final char tmp = str[low];
      str[low] = str[high];
      str[high] = tmp;
    }
  }

  static void run(char[] str, int runs) {
    while (runs-- > 0) reverse(str);

    reverse(str);

    if (!new String(str).equals("fedcba9876543210")) {
      throw new AssertionError("reverse-string output mismatch");
    }
  }

  public static void main(String[] args) {
    run("0123456789abcdef".toCharArray(), runs);
  }
}
