// BOT-TRANSLATED

final class bubble {
  static volatile int length = 32768;

  static long pseudoRandom(long seed) {
    return (seed * 1309 + 13849) & 65535;
  }

  static void listInit(long[] list) {
    long seed = 74755;

    for (int ind = 0; ind < list.length; ind++) {
      list[ind] = seed = pseudoRandom(seed);
    }
  }

  static void listVerify(long[] list) {
    for (int ind = 0; ind < list.length - 1; ind++) {
      if (list[ind] > list[ind + 1]) {
        throw new AssertionError("[bubble] not sorted");
      }
    }
  }

  static void bubble(long[] list) {
    for (int ceil = list.length - 1; ceil > 0; ceil--) {
      for (int ind = 0; ind < ceil; ind++) {
        if (list[ind] > list[ind + 1]) {
          final long tmp = list[ind];
          list[ind] = list[ind + 1];
          list[ind + 1] = tmp;
        }
      }
    }
  }

  static void bubbleSort(int length) {
    final long[] list = new long[length];
    listInit(list);
    bubble(list);
    listVerify(list);
  }

  public static void main(String[] args) {
    bubbleSort(length);
  }
}
