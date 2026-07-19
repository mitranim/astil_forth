// BOT-TRANSLATED

import java.util.Arrays;

final class sieve {
  static volatile int capacity = 8192;
  static volatile int runs = 16384;

  static int findPrime(byte[] flags) {
    Arrays.fill(flags, (byte) 1);

    int num = 0;
    int step = 3;

    for (int ind = 0; ind < flags.length; ind++) {
      if (flags[ind] != 0) {
        for (int ind1 = ind + step; ind1 < flags.length; ind1 += step) {
          flags[ind1] = 0;
        }

        num++;
      }

      step += 2;
    }

    return num;
  }

  static void run(int capacity, int runs) {
    final byte[] flags = new byte[capacity];
    int out = 0;

    while (runs-- > 0) out = findPrime(flags);

    if (out != 1899) {
      throw new AssertionError("mismatch: expected 1899; got " + out);
    }
  }

  public static void main(String[] args) {
    run(capacity, runs);
  }
}
