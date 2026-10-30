#include "ReactorNet/net/EventLoop.h"
#include "ReactorNet/net/TcpClient.h"
#include "ReactorNet/log/Logging.h"
#include <iostream>
#include <thread>
int main(int argc, char** argv) {
  if (argc != 3) return 1;
  reactor::Logger::setLogLevel(reactor::Logger::WARN);
  reactor::net::EventLoop loop;
  const std::string host = argv[1], payload(262144, 'c');
  reactor::net::InetAddress address(host, static_cast<uint16_t>(std::stoi(argv[2])), host.find(':') != std::string::npos);
  reactor::net::TcpClient client(&loop, address, "client-test");
  std::string received;
  bool timedOut = false, disconnected = false;
  client.setConnectionCallback([&](const reactor::net::TcpConnectionPtr& connection) {
    if (connection->connected()) {
      connection->setTcpNoDelay(true);
      std::thread sender([connection, &payload] {
        connection->send(payload);
        connection->shutdown();
      });
      sender.join();
    } else {
      disconnected = true;
      loop.queueInLoop([&] { loop.quit(); });
    }
  });
  client.setMessageCallback([&](const reactor::net::TcpConnectionPtr&, reactor::net::Buffer* buffer, reactor::Timestamp) {
    received += buffer->retrieveAllAsString();
  });
  loop.runAfter(5, [&] { timedOut = true; loop.quit(); });
  client.connect();
  loop.loop();
  if (timedOut || !disconnected || received != payload) {
    std::cerr << "TcpClient cross-thread send or graceful close failed\n";
    return 1;
  }
}
