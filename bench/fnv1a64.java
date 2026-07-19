// BOT-TRANSLATED

final class fnv1a64 {
  static final long OFFSET = 0xcbf29ce484222325L;
  static final long PRIME = 0x100000001b3L;
  static final long WANT = 0xb0a1ea8560222325L;
  static volatile int capacity = 65536;
  static volatile int runs = 2048;

  static void init(byte[] buf) {
    final String pattern = "0123456789abcdef";

    for (int ind = 0; ind < buf.length; ind++) {
      buf[ind] = (byte) pattern.charAt(ind & 15);
    }
  }

  static long hash(long hash, byte[] src) {
    for (final byte val : src) {
      hash ^= val & 0xffL;
      hash *= PRIME;
    }

    return hash;
  }

  static void run(int capacity, int runs) {
    final byte[] buf = new byte[capacity];
    init(buf);

    long hash = OFFSET;
    while (runs-- > 0) hash = hash(hash, buf);

    if (hash != WANT) {
      throw new AssertionError(
        "mismatch: expected 0x"
          + Long.toHexString(WANT)
          + "; got 0x"
          + Long.toHexString(hash)
      );
    }
  }

  public static void main(String[] args) {
    run(capacity, runs);
  }
}
