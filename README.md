# ReactorNet

基于 Linux Reactor 的 C++11 网络库：非阻塞 TCP、epoll/poll、每个 I/O 线程一个 EventLoop、
eventfd 跨线程唤醒、timerfd 定时器、连接缓冲区和异步日志。

通用设施使用 C++11 标准库的线程、互斥锁、条件变量、原子变量、智能指针、容器和 chrono。
核心库不依赖 Boost。网络 I/O 与平台信息仍使用 Linux/POSIX 接口。

## 构建与测试

要求 Linux/WSL、支持 C++11 的 GCC/Clang、CMake >= 3.16，以及 Make 或 Ninja。
回归测试额外需要 Python 3。压缩扩展 `GzipFile` / `ZlibStream` 可选依赖 zlib；核心库不依赖它。

```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
ctest --test-dir build/release --output-on-failure
```

只构建库可设置 `-DBUILD_TESTING=OFF -DREACTORNET_BUILD_BENCHMARKS=OFF`。
不使用 Ninja 时省略 `-G Ninja`。Windows 开发时在 WSL 终端进入项目目录执行上述命令。

```bash
cmake -S . -B build/asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DREACTORNET_SANITIZERS=ON
cmake --build build/asan -j
ctest --test-dir build/asan --output-on-failure
```

CMake 目标为 `ReactorNet::base`、`ReactorNet::log`、`ReactorNet::net`。
在使用 `add_subdirectory` 引入项目的应用中，链接 `ReactorNet::net` 即可传递链接其基础依赖。

## 最小 echo 服务与性能测试

终端 A：

```bash
./build/release/bin/reactornet_echo_server 9981 4 127.0.0.1
```

终端 B：

```bash
./build/release/bin/reactornet_bench_client 127.0.0.1 9981 64 1024 30 4 5
```

上述配置使用 4 个 I/O 工作线程、64 条连接、1 KiB 消息、4 个客户端线程，预热 5 秒后计时 30 秒。
输出吞吐、错误数、平均 RTT 及 p50/p95/p99。`setThreadNum(0)` 使用主 EventLoop 处理所有 I/O。

完整矩阵、CPU/网络瓶颈分析与测量限制见 [性能测试指南](docs/PERFORMANCE.md)。
版本与接口迁移见 [C++11 重构说明](docs/MIGRATION.md)。

## 使用约定

TCP 是字节流，应用需要自己定义消息边界；`Buffer` 不会自动完成任意协议的分包。
连接回调在所属 EventLoop 中串行运行，避免执行阻塞操作；耗时业务可交给 `ThreadPool`，
再向连接所属循环投递结果。EventLoop/连接/服务端的生命周期必须覆盖投递的异步操作。
异步日志应在启动后追加，并在退出前 stop，以刷新尾部缓冲。

回归测试覆盖标准库组件、线程启动与失败恢复、定时器、跨线程唤醒、异步日志刷新，
以及 epoll/poll 后端的 IPv4/IPv6、多连接、大消息、半关闭、重置和 TcpClient 跨线程发送。
所有库头文件分别单独编译。已有源码中的版权与许可证声明保留。
