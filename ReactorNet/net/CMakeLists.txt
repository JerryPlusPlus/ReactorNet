add_library(reactornet_net
  Acceptor.cc Buffer.cc Channel.cc Connector.cc EventLoop.cc EventLoopThread.cc
  EventLoopThreadPool.cc InetAddress.cc Poller.cc poller/DefaultPoller.cc
  poller/EPollPoller.cc poller/PollPoller.cc Socket.cc SocketsOps.cc
  TcpClient.cc TcpConnection.cc TcpServer.cc Timer.cc TimerQueue.cc)
add_library(ReactorNet::net ALIAS reactornet_net)
target_link_libraries(reactornet_net PUBLIC ReactorNet::base ReactorNet::log)
target_compile_options(reactornet_net PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS reactornet_net ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
