// BOT-TRANSLATED

import java.math.BigInteger;

final class fib_loop_big {
  static final BigInteger WANT =
    new BigInteger("205697230343233228174223751303346572685");
  static volatile int depth = 184;
  static volatile int runs = 1 << 21;

  static BigInteger fib(int depth) {
    BigInteger prev = BigInteger.ZERO;
    BigInteger next = BigInteger.ONE;

    while (depth-- > 0) {
      final BigInteger tmp = prev.add(next);
      prev = next;
      next = tmp;
    }

    return next;
  }

  static void run(int runs) {
    BigInteger out = BigInteger.ZERO;

    while (runs-- > 0) out = fib(depth);

    if (!out.equals(WANT)) {
      throw new AssertionError("big iterative Fibonacci output mismatch");
    }
  }

  public static void main(String[] args) {
    run(runs);
  }
}
