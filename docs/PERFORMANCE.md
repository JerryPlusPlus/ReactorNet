# ReactorNet 性能测试

以下命令在 Linux/WSL 的项目根目录执行。计时使用 `std::chrono::steady_clock`。
先确认正确性，再测吞吐和延迟。Debug、ASan/UBSan 的结果不能用作发布性能数据。

## 1. 构建与正确性验证

```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
ctest --test-dir build/release --output-on-failure
```

这会构建三个库、两个性能程序和回归测试；全部头文件也分别单独编译。
测试覆盖队列、线程/线程池、TLS、上下文、定时器、事件循环、日志尾部刷新，
以及 epoll/poll + IPv4/IPv6 下的大消息、半关闭、重置和跨线程发送。

```bash
cmake -S . -B build/asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DREACTORNET_SANITIZERS=ON
cmake --build build/asan -j
ctest --test-dir build/asan --output-on-failure
```

ASan/UBSan 主要检查越界、悬空引用、泄漏和未定义行为，并不检测所有数据竞争。

## 2. 启动 echo 服务

终端 A：

```bash
./build/release/bin/reactornet_echo_server 9981 4 127.0.0.1
```

参数为 `端口 I/O线程数 监听IP`。`0` 表示在主 EventLoop 线程处理所有 I/O；
`4` 表示一个接收连接的主线程和四个 I/O 工作线程。默认关闭逐连接 INFO 日志，开启 TCP_NODELAY。
远程压测时改为服务器的指定网卡地址；只在可信测试网络开放该端口。

终端 B：

```bash
./build/release/bin/reactornet_bench_client 127.0.0.1 9981 64 1024 30 4 5
```

参数依次为：`目标IP 端口 连接数 每次消息字节数 计时秒数 客户端线程数 预热秒数`。
默认客户端线程数为 1，预热 3 秒。消息大小上限为 1 MiB；每条连接只有一个未完成请求。
客户端校验每个回包的内容，连接失败、断开和内容不符计入 `errors`；有错误或零完成请求时返回非零。
建立连接和预热时间不计入吞吐，跨过预热边界的请求也不计入。

输出字段：

| 字段 | 含义 |
| --- | --- |
| `requests_per_sec` | 每秒完成的固定长度 echo 往返数 |
| `echo_mib_per_sec` | 单方向有效负载吞吐；1 MiB = 1,048,576 字节 |
| `bidirectional_mib_per_sec` | 双向有效负载合计，等于前者两倍，不含 TCP/IP 开销 |
| `mean_rtt_us` | 所有完成请求的平均 RTT，单位微秒 |
| `p50/p95/p99_rtt_us` | 完成请求的 RTT 分位数，包含客户端排队与调度时间 |
| `latency_samples` | 分位数使用的样本数；每客户端线程最多均匀采样 100 万条，按各线程请求量加权合并 |
| `errors` | 测试期间发生的连接/协议错误，期望为 0 |

这是闭环压测：服务变慢时，客户端也会降低提交速率。它不能代替固定到达率压测，
不能用其 p99 推断过载时的排队延迟；未完成请求不进入 RTT 分位数。
报告中必须同时保留吞吐、错误数、连接数和客户端配置。

## 3. 建议测试矩阵

| 变量 | 示例 |
| --- | --- |
| I/O 工作线程 | 0、1、2、4、8，按物理核心数选择 |
| 持续连接数 | 1、16、64、256、1,024，逐步增加 |
| 消息大小 | 64 B、1 KiB、16 KiB、64 KiB |
| 后端 | 默认 epoll；设置 `REACTORNET_USE_POLL=1` 使用 poll |
| 单次时长 | 预热 5 秒，测量 30–60 秒 |
| 重复次数 | 每组至少 5 次，报告中位数和离散程度 |

每次只改变一个变量。不要把一次小消息请求的 requests/s 与大消息 requests/s 直接比较。
高连接数需要先确认 `ulimit -n`，并检查客户端的临时端口、CPU 和内存是否先达到瓶颈。

```bash
# 停止之前的服务实例后，在终端 A 运行 poll 版本。
REACTORNET_USE_POLL=1 ./build/release/bin/reactornet_echo_server 9981 4 127.0.0.1
```

另外分别测试持续连接、连接建立/关闭速率、空闲连接容量、慢读客户端和长时间运行。
当前内置客户端针对持续连接 RTT/吞吐；连接风暴和固定到达率过载测试应使用专门的发生器，
不能把内置结果称为整个网络库的性能上限。

## 4. 采集资源与定位瓶颈

在具备足够独立 CPU 核心的机器上，可分别约束服务与发生器的 CPU 集合，避免互相抢占：

```bash
# CPU 编号仅是示例，先用 lscpu 检查本机核心与 SMT/NUMA 拓扑。
taskset -c 0-4 ./build/release/bin/reactornet_echo_server 9981 4 127.0.0.1
taskset -c 5-8 ./build/release/bin/reactornet_bench_client 127.0.0.1 9981 256 1024 30 4 5
```

在压测已进入测量阶段时，对服务 PID 采集软件计数器：

```bash
perf stat -p SERVER_PID -e task-clock,context-switches,cpu-migrations,page-faults -- sleep 30
```

有 PMU 支持和访问权限时再增加 `cycles,instructions,cache-misses`。
同时记录进程 CPU/RSS、连接数、重传、收发字节、网卡速率，检查压测端是否已经满载。
CPU 亲和性及计数器选项可参考 [taskset 手册](https://man7.org/linux/man-pages/man1/taskset.1.html)
与 [perf stat 手册](https://man7.org/linux/man-pages/man1/perf-stat.1.html)。

正式网络吞吐测试使用两台独立 Linux 主机与真实网卡，记录 CPU、内核、编译器、构建选项、
网卡/MTU、TCP 设置、拓扑和日志策略。本机回环同时消耗服务端与客户端 CPU；
WSL 的虚拟化与文件系统也会影响结果。单独测异步日志吞吐、队列/线程池开销时，不要混入网络 RTT。

## 5. 比较重构前后

同一硬件、编译参数、连接/消息配置、CPU 分配和日志配置下重复测试两个版本。
先比较错误数与正确性，再比较吞吐和 p99；采用固定流量时还要比较排队与丢弃。
本次重构前的仓库无法完整构建，不能直接据此宣称性能提升或退化。
仓库内的短时 smoke 结果只验证工具可运行，不能作为正式性能结论。
