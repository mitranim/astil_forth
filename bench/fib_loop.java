// BOT-TRANSLATED

final class fib_loop {
  static volatile int depth = 91;
  static volatile int runs = 1 << 22;

  static long fib(int depth) {
    long prev = 0;
    long next = 1;

    while (depth-- > 0) {
      final long tmp = prev + next;
      prev = next;
      next = tmp;
    }

    return next;
  }

  public static void main(String[] args) {
    final int depth = fib_loop.depth;
    int runs = fib_loop.runs;
    long out = 0;

    while (runs-- > 0) out = fib(depth);

    if (out != 7540113804746346429L) {
      throw new AssertionError("iterative Fibonacci output mismatch");
    }
  }
}
