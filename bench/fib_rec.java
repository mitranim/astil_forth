// BOT-TRANSLATED

final class fib_rec {
  static volatile long depth = 39;

  static long fib(long ind) {
    if (ind <= 1) return 1;
    return fib(ind - 1) + fib(ind - 2);
  }

  public static void main(String[] args) {
    if (fib(depth) != 102334155) {
      throw new AssertionError("recursive Fibonacci output mismatch");
    }
  }
}
