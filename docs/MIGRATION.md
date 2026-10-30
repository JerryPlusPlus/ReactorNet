# C++11 重构说明

项目继续要求 C++11，不使用 C++14 的 `make_unique`、泛型 lambda 或 C++17 的 `std::any`/`string_view`。

| 原实现 | 当前实现 |
| --- | --- |
| pthread 创建/join/detach | `std::thread`，保留 `reactor::Thread` 的 TID、名称与启动同步 |
| MutexLock/Condition/CountDownLatch | `std::mutex`、RAII 锁、带谓词的 `std::condition_variable` |
| AtomicInt32/64 | `std::atomic` |
| pthread_once | `std::call_once` / `std::once_flag` |
| pthread_key 与 `__thread` | `thread_local` 与标准容器/智能指针 |
| Boost circular_buffer | `std::deque` 与显式容量限制 |
| Boost operators | 显式比较运算符 |
| Boost any | `reactor::Any`，使用 `std::unique_ptr`、`type_info` 和类型擦除实现 C++11 值语义 |
| gettimeofday / nanosleep | `std::chrono::system_clock` / `std::this_thread::sleep_for` |

`ThreadLocal<T>` 的不同实例使用独立键，值在线程退出时释放。实例销毁后，其他线程持有的值
也会保留到对应线程退出；调用 `value()` 时实例必须仍然存活。
单例的 `no_destroy` 兼容约定仍受支持。

上下文迁移示例：

```cpp
connection->setContext(std::string("session"));
const auto& text = reactor::any_cast<const std::string&>(connection->getContext());
auto* mutableText = reactor::any_cast<std::string>(connection->getMutableContext());
```

`Any` 要求存储对象可拷贝，错误的指针转换返回 `nullptr`，错误的值转换抛 `std::bad_cast`。
把旧的 `boost::any_cast` 改为 `reactor::any_cast`；`StringPiece` 继续提供非拥有字符串视图，
底层字符串/缓冲区必须比视图活得更久。

`Thread::join()` 返回 `void`，线程创建错误通过 `std::system_error` 报告。
线程池的 `start()/stop()` 由外部线程串行调用，任务异常保持原来的 fail-fast 行为；
`stop()` 等待正在运行的任务，不保证执行队列中尚未开始的任务，并会唤醒被满队列阻塞的生产者。
零工作线程模式保留在调用线程直接执行任务的行为。
异步日志只在 `start()` 与 `stop()` 之间接收追加，`stop()` 刷新已有缓冲；超大积压仍有丢弃策略。

网络核心仍依赖 Linux 的 socket、epoll/poll、eventfd、timerfd、TID 和线程命名接口。
`pthread_atfork` 用于 fork 后重置 TID 缓存；C++11 标准库没有对应接口。
Linux 栈回溯、进程信息和时区文件解析也仍属于平台层。
这些接口不是线程/容器库的替代品，保留它们不影响核心移除 Boost 依赖。
`GzipFile.h` / `ZlibStream.h` 是可选 zlib 扩展，使用它们时需安装 zlib 并链接 `-lz`。

构建统一为 CMake，三个目标为 `ReactorNet::base`、`ReactorNet::log`、`ReactorNet::net`。
启用 `BUILD_TESTING` 需要 Python 3；仅构建库可设置 `-DBUILD_TESTING=OFF`。
