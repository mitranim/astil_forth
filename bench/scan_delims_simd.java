// BOT-TRANSLATED

import jdk.incubator.vector.ByteVector;
import jdk.incubator.vector.VectorSpecies;

final class scan_delims_simd {
  static final VectorSpecies<Byte> SPECIES = ByteVector.SPECIES_128;
  static volatile byte[] input;
  static volatile int capacity = 1 << 16;
  static volatile int runs = 1 << 15;

  static int scan(byte[] input) {
    final int bound = SPECIES.loopBound(input.length);
    int out = 0;
    int ind = 0;

    for (; ind < bound; ind += SPECIES.length()) {
      final ByteVector vec = ByteVector.fromArray(SPECIES, input, ind);
      var matches = vec.eq((byte) '{');
      matches = matches.or(vec.eq((byte) '}'));
      matches = matches.or(vec.eq((byte) '['));
      matches = matches.or(vec.eq((byte) ']'));
      matches = matches.or(vec.eq((byte) ':'));
      matches = matches.or(vec.eq((byte) ','));
      matches = matches.or(vec.eq((byte) ' '));
      matches = matches.or(vec.eq((byte) '\n'));
      matches = matches.or(vec.eq((byte) '\t'));
      out += matches.trueCount();
    }

    for (; ind < input.length; ind++) {
      out += scan_delims.DELIMITERS[input[ind] & 0xff];
    }

    return out;
  }

  static void run(int runs) {
    long out = 0;

    while (runs-- > 0) out += scan(input);

    if (out != scan_delims.WANT) {
      throw new AssertionError(
        "mismatch: expected " + scan_delims.WANT + "; got " + out
      );
    }
  }

  public static void main(String[] args) {
    input = scan_delims.initInput(capacity);
    run(runs);
  }
}
