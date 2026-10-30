#include "ReactorNet/net/EventLoop.h"
#include "ReactorNet/net/InetAddress.h"
#include "ReactorNet/net/TcpServer.h"
#include "ReactorNet/log/Logging.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
  try {
    const int port = argc > 1 ? std::stoi(argv[1]) : 9981;
    const int threads = argc > 2 ? std::stoi(argv[2]) : 0;
    const std::string host = argc > 3 ? argv[3] : "127.0.0.1";
    if (port < 1 || port > 65535 || threads < 0) throw std::invalid_argument("invalid port or thread count");
    reactor::Logger::setLogLevel(reactor::Logger::WARN);
    reactor::net::EventLoop loop;
    reactor::net::InetAddress address(host, static_cast<uint16_t>(port), host.find(':') != std::string::npos);
    reactor::net::TcpServer server(&loop, address, "echo");
    server.setThreadNum(threads);
    server.setConnectionCallback([](const reactor::net::TcpConnectionPtr& connection) {
      if (connection->connected()) connection->setTcpNoDelay(true);
    });
    server.setMessageCallback([](const reactor::net::TcpConnectionPtr& connection,
                                 reactor::net::Buffer* buffer, reactor::Timestamp) {
      connection->send(buffer);
    });
    server.start();
    std::cout << "listening=" << address.toIpPort() << " io_threads=" << threads << std::endl;
    loop.loop();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
