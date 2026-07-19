// BOT-ASSISTED

final class scan_delims {
  static final String PATTERN = "{a,b:c[d]e} \n\tfg";
  static final byte[] DELIMITERS = initDelimiters();
  static final long WANT = 1207959552L;
  static volatile byte[] input;
  static volatile int capacity = 1 << 16;
  static volatile int runs = 1 << 15;

  static byte[] initDelimiters() {
    final byte[] delimiters = new byte[256];

    for (final char val : "{}[]:, \n\t".toCharArray()) {
      delimiters[val] = 1;
    }

    return delimiters;
  }

  static byte[] initInput(int capacity) {
    final byte[] input = new byte[capacity];

    for (int ind = 0; ind < input.length; ind++) {
      input[ind] = (byte) PATTERN.charAt(ind & 15);
    }

    return input;
  }

  static int scan(byte[] input, byte[] delimiters) {
    int out = 0;

    for (final byte val : input) {
      out += delimiters[val & 0xff];
    }

    return out;
  }

  static void run(int runs) {
    long out = 0;

    while (runs-- > 0) out += scan(input, DELIMITERS);

    if (out != WANT) {
      throw new AssertionError("mismatch: expected " + WANT + "; got " + out);
    }
  }

  public static void main(String[] args) {
    input = initInput(capacity);
    run(runs);
  }
}
