// BOT-GENERATED

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

final class cat {
  private static void copy(
    FileChannel input, FileChannel output, ByteBuffer buffer
  ) throws IOException {
    while (input.read(buffer) != -1) {
      buffer.flip();
      while (buffer.hasRemaining()) {
        output.write(buffer);
      }
      buffer.clear();
    }
  }

  public static void main(String[] args) throws IOException {
    var stdin = new FileInputStream(FileDescriptor.in);
    var stdout = new FileOutputStream(FileDescriptor.out);
    var output = stdout.getChannel();

    /*
    Why an explicit 64 KiB loop:
    - `transferTo(System.out)` falls back on JDK's generic 16 KiB loop,
      which takes longer than with our 64 KiB buffer.
    - `transferTo(rawStdout)` can `mmap` regular files. `/dev/null` may
      discard those mappings without faulting the input pages, giving
      fake throughput during the benchmark; that path is also slower
      for real pipe and file outputs.
    - A `byte[]` loop is slower here. A direct buffer lets `FileChannel`
      avoid an intermediate buffer around native I/O.
    */
    var buffer = ByteBuffer.allocateDirect(64 * 1024);

    if (args.length == 0) {
      copy(stdin.getChannel(), output, buffer);
    }

    for (String name : args) {
      if (name.equals("-")) {
        copy(stdin.getChannel(), output, buffer);
        continue;
      }

      try (var input = new FileInputStream(name)) {
        copy(input.getChannel(), output, buffer);
      }
    }

    stdout.flush();
  }
}
