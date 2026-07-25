// BOT-GENERATED

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.StandardSocketOptions;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousServerSocketChannel;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.util.concurrent.CountDownLatch;

final class tcp_server_async {
  private static final byte READY = 'R';
  private static final byte DATA = 'D';
  private static final byte CLOSE = 'Q';
  private static final int PORT = 19777;

  private static final int WRITING_READY = 0;
  private static final int READING_DATA = 1;
  private static final int WRITING_ECHO = 2;

  private static final class Client {
    final AsynchronousSocketChannel socket;
    final ByteBuffer byteBuffer = ByteBuffer.allocate(1);
    int operation = WRITING_READY;
    boolean closeAfterWrite;

    Client(AsynchronousSocketChannel socket) {
      this.socket = socket;
      byteBuffer.put(READY).flip();
    }
  }

  private static void fail(Throwable error) {
    error.printStackTrace();
    System.exit(1);
  }

  private static void close(Client client) {
    try {
      client.socket.close();
    } catch (IOException error) {
      fail(error);
    }
  }

  private static void continueIo(Client client) {
    switch (client.operation) {
      case WRITING_READY, WRITING_ECHO ->
        client.socket.write(client.byteBuffer, client, IO_HANDLER);
      case READING_DATA ->
        client.socket.read(client.byteBuffer, client, IO_HANDLER);
      default -> throw new AssertionError("bad async operation");
    }
  }

  private static final CompletionHandler<Integer, Client> IO_HANDLER =
    new CompletionHandler<>() {
      @Override
      public void completed(Integer count, Client client) {
        if (count < 0) {
          fail(new IOException("short TCP read"));
          return;
        }
        if (client.byteBuffer.hasRemaining()) {
          continueIo(client);
          return;
        }

        switch (client.operation) {
          case WRITING_READY -> {
            client.operation = READING_DATA;
            client.byteBuffer.clear();
            continueIo(client);
          }
          case READING_DATA -> {
            client.byteBuffer.flip();
            final byte data = client.byteBuffer.get();
            if (data != DATA && data != CLOSE) {
              fail(new AssertionError("bad data byte"));
              return;
            }
            client.closeAfterWrite = data == CLOSE;
            client.operation = WRITING_ECHO;
            client.byteBuffer.clear();
            client.byteBuffer.put(data).flip();
            continueIo(client);
          }
          case WRITING_ECHO -> {
            if (client.closeAfterWrite) {
              close(client);
            } else {
              client.operation = READING_DATA;
              client.byteBuffer.clear();
              continueIo(client);
            }
          }
          default -> throw new AssertionError("bad async operation");
        }
      }

      @Override
      public void failed(Throwable error, Client client) {
        close(client);
        fail(error);
      }
    };

  private static final CompletionHandler<
    AsynchronousSocketChannel,
    AsynchronousServerSocketChannel
  > ACCEPT_HANDLER = new CompletionHandler<>() {
    @Override
    public void completed(
      AsynchronousSocketChannel socket,
      AsynchronousServerSocketChannel listener
    ) {
      listener.accept(listener, this);
      continueIo(new Client(socket));
    }

    @Override
    public void failed(
      Throwable error,
      AsynchronousServerSocketChannel listener
    ) {
      fail(error);
    }
  };

  public static void main(String[] args) throws Exception {
    try (var listener = AsynchronousServerSocketChannel.open()) {
      listener.setOption(StandardSocketOptions.SO_REUSEADDR, true);
      listener.bind(new InetSocketAddress("127.0.0.1", PORT), 128);
      listener.accept(listener, ACCEPT_HANDLER);
      new CountDownLatch(1).await();
    }
  }
}
