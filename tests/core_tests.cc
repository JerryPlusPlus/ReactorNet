#include "ReactorNet/base/Any.h"
#include "ReactorNet/base/BlockingQueue.h"
#include "ReactorNet/base/BoundedBlockingQueue.h"
#include "ReactorNet/base/Singleton.h"
#include "ReactorNet/base/ThreadLocal.h"
#include "ReactorNet/base/ThreadLocalSingleton.h"
#include "ReactorNet/base/Exception.h"
#include "ReactorNet/base/Date.h"
#include "ReactorNet/log/AsyncLogging.h"
#include "ReactorNet/log/Logging.h"
#include "ReactorNet/net/Buffer.h"
#include "ReactorNet/net/EventLoop.h"
#include "ReactorNet/net/EventLoopThread.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <future>
#include <glob.h>
#include <iterator>
#include <thread>
#include <unistd.h>

void check(bool value, const char* message) {
  if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::abort(); }
}
struct Tracked {
  static std::atomic<int> created, destroyed;
  Tracked() { ++created; }
  ~Tracked() { ++destroyed; }
};
std::atomic<int> Tracked::created{0}, Tracked::destroyed{0};
int main() {
  reactor::Logger::setLogLevel(reactor::Logger::WARN);
  reactor::Any value(std::string("context"));
  reactor::Any copy = value;
  reactor::any_cast<std::string&>(copy) += " copy";
  check(reactor::any_cast<std::string>(value) == "context", "context has value semantics");
  check(reactor::any_cast<int>(&value) == nullptr, "context rejects wrong types");
  const reactor::Any number(42);
  check(*reactor::any_cast<const int>(&number) == 42, "const context cast");
  check(reactor::Any().empty(), "empty context");
  bool badCast = false;
  try { reactor::any_cast<int>(value); } catch (const std::bad_cast&) { badCast = true; }
  check(badCast, "bad value cast throws");

  reactor::BlockingQueue<std::unique_ptr<int>> queue;
  std::thread producer([&] { queue.put(std::unique_ptr<int>(new int(7))); });
  check(*queue.take() == 7, "move-only unbounded queue");
  producer.join();
  reactor::BoundedBlockingQueue<std::unique_ptr<int>> bounded(1);
  bounded.put(std::unique_ptr<int>(new int(1)));
  std::promise<void> submitted;
  auto future = submitted.get_future();
  std::thread blocked([&] { bounded.put(std::unique_ptr<int>(new int(2))); submitted.set_value(); });
  check(future.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout, "bounded put blocks");
  check(*bounded.take() == 1, "bounded FIFO first");
  check(future.wait_for(std::chrono::seconds(2)) == std::future_status::ready, "bounded put wakes");
  check(*bounded.take() == 2, "bounded FIFO second");
  blocked.join();

  reactor::ThreadLocal<int> first, second;
  first.value() = 9;
  std::thread local([&] {
    check(first.value() == 0 && second.value() == 0, "per-instance per-thread state");
    first.value() = 3;
    check(second.value() == 0, "thread local keys do not alias");
    check(reactor::ThreadLocalSingleton<Tracked>::pointer() == nullptr, "lazy TLS singleton");
    reactor::ThreadLocalSingleton<Tracked>::instance();
    reactor::Singleton<Tracked>::instance();
  });
  local.join();
  check(first.value() == 9 && Tracked::destroyed == 1, "TLS isolation and destruction");
  reactor::Singleton<Tracked>::instance();
  check(Tracked::created == 2, "singleton is constructed only once");

  reactor::net::Buffer buffer;
  buffer.append("hello", 5);
  buffer.appendInt32(1234);
  check(buffer.retrieveAsString(5) == "hello", "buffer byte sequence");
  check(buffer.readInt32() == 1234 && buffer.readableBytes() == 0, "buffer network integer");
  check(reactor::Date(2024, 2, 29).toIsoString() == "2024-02-29", "calendar date");
  const auto now = reactor::Timestamp::now();
  check(now.valid() && now <= reactor::Timestamp::now(), "chrono timestamp");
  check(reactor::Exception("test").stackTrace()[0] != '\0', "exception stack trace is available");

  {
    reactor::net::EventLoop loop;
    int repeats = 0;
    bool canceled = true;
    auto once = loop.runAfter(0.01, [&] { canceled = false; });
    loop.cancel(once);
    reactor::net::TimerId repeating;
    repeating = loop.runEvery(0.005, [&] { if (++repeats == 3) loop.cancel(repeating); });
    loop.runAfter(0.04, [&] { loop.quit(); });
    loop.loop();
    check(canceled && repeats == 3, "timer firing, recurrence, and cancellation");
  }
  {
    reactor::net::EventLoop loop;
    loop.quit();
    loop.loop();
  }
  {
    reactor::net::EventLoopThread loopThread;
    auto* loop = loopThread.startLoop();
    std::promise<void> ran;
    auto done = ran.get_future();
    loop->queueInLoop([&] { ran.set_value(); });
    check(done.wait_for(std::chrono::seconds(2)) == std::future_status::ready, "cross-thread wakeup");
  }

  const std::string basename = "async-test-" + std::to_string(::getpid());
  {
    reactor::AsyncLogging logging(basename, 10000000, 30);
    logging.start();
    const std::string tail = "tail must survive immediate stop\n";
    for (int i = 0; i < 100; ++i) logging.append(tail.data(), static_cast<int>(tail.size()));
    logging.stop();
    logging.stop();
  }
  glob_t files{};
  check(::glob((basename + ".*.log").c_str(), 0, nullptr, &files) == 0, "log file exists");
  std::string content;
  for (size_t i = 0; i < files.gl_pathc; ++i) {
    std::ifstream input(files.gl_pathv[i]);
    content.append(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    std::remove(files.gl_pathv[i]);
  }
  ::globfree(&files);
  size_t lines = 0;
  for (char ch : content) if (ch == '\n') ++lines;
  check(lines == 100, "asynchronous stop flushes every trailing record");
  std::puts("PASS: context, queues, TLS, singleton, buffers, timers, wakeup, asynchronous flush");
}
