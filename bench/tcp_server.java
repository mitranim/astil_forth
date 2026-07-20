// BOT-GENERATED

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class tcp_server {
  private static final int READY = 'R';
  private static final int DATA = 'D';
  private static final int PORT = 19777;

  private static void handle(Socket socket) {
    try (socket) {
      final var out = socket.getOutputStream();
      final var in = socket.getInputStream();
      out.write(READY);
      if (in.read() != DATA) throw new AssertionError("bad DATA byte");
      out.write(DATA);
    } catch (Exception error) {
      throw new RuntimeException(error);
    }
  }

  static void run(ExecutorService executor) throws Exception {
    try (var listener = new ServerSocket(); executor) {
      listener.setReuseAddress(true);
      listener.bind(
        new InetSocketAddress(InetAddress.getLoopbackAddress(), PORT)
      );
      while (true) {
        final Socket socket = listener.accept();
        executor.execute(() -> handle(socket));
      }
    }
  }

  public static void main(String[] args) throws Exception {
    run(Executors.newVirtualThreadPerTaskExecutor());
  }
}

final class tcp_server_thread {
  public static void main(String[] args) throws Exception {
    tcp_server.run(
      Executors.newThreadPerTaskExecutor(Thread.ofPlatform().factory())
    );
  }
}
