#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

using Clock = std::chrono::steady_clock;
struct Connection {
  int fd;
  size_t sent{0}, received{0};
  Clock::time_point began;
  explicit Connection(int socket) : fd(socket) {}
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection(Connection&& other) noexcept : fd(other.fd) { other.fd = -1; }
  ~Connection() { if (fd >= 0) ::close(fd); }
};
struct Result {
  uint64_t requests{0}, errors{0};
  double totalLatency{0};
  std::vector<double> samples;
  std::string failure;
};

int main(int argc, char** argv) {
  try {
    if (argc < 6) {
      std::cerr << "usage: " << argv[0] << " HOST PORT CONNECTIONS BYTES SECONDS [CLIENT_THREADS=1] [WARMUP=3]\n";
      return 1;
    }
    const std::string host = argv[1], port = argv[2];
    const int count = std::stoi(argv[3]), bytes = std::stoi(argv[4]);
    const double seconds = std::stod(argv[5]);
    const int threads = argc > 6 ? std::stoi(argv[6]) : 1;
    const double warmup = argc > 7 ? std::stod(argv[7]) : 3.0;
    if (count < 1 || bytes < 1 || bytes > 1024 * 1024 || seconds <= 0 || warmup < 0 || threads < 1 || threads > count)
      throw std::invalid_argument("invalid benchmark parameters");
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* rawAddress = nullptr;
    const int resolved = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &rawAddress);
    if (resolved) throw std::runtime_error(::gai_strerror(resolved));
    std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> address(rawAddress, &::freeaddrinfo);
    std::promise<Clock::time_point> startPromise;
    auto start = startPromise.get_future().share();
    std::mutex readyMutex;
    std::condition_variable readyCondition;
    int ready = 0;
    std::vector<Result> results(static_cast<size_t>(threads));
    std::vector<std::thread> workers;
    const std::string payload(static_cast<size_t>(bytes), 'x');
    for (int worker = 0; worker < threads; ++worker) {
      workers.emplace_back([&, worker] {
        Result& result = results[worker];
        bool announced = false;
        auto announce = [&] {
          std::lock_guard<std::mutex> lock(readyMutex);
          ++ready;
          announced = true;
          readyCondition.notify_one();
        };
        try {
          const int localCount = count / threads + (worker < count % threads ? 1 : 0);
          std::vector<Connection> connections;
          connections.reserve(static_cast<size_t>(localCount));
          for (int i = 0; i < localCount; ++i) {
            const int fd = ::socket(address->ai_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
            if (fd < 0) throw std::runtime_error("socket: " + std::string(std::strerror(errno)));
            connections.emplace_back(fd);
            const int enabled = 1;
            ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof enabled);
            if (::connect(fd, address->ai_addr, address->ai_addrlen) < 0) {
              if (errno != EINPROGRESS) throw std::runtime_error("connect: " + std::string(std::strerror(errno)));
              pollfd descriptor{fd, POLLOUT, 0};
              int error = 0;
              socklen_t length = sizeof error;
              if (::poll(&descriptor, 1, 5000) <= 0 || ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0 || error)
                throw std::runtime_error("connection could not be established");
            }
          }
          announce();
          const auto measuredStart = start.get();
          const auto finish = measuredStart + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
          for (auto& connection : connections) connection.began = Clock::now();
          std::vector<pollfd> descriptors(connections.size());
          std::minstd_rand random(static_cast<unsigned>(worker + 1));
          result.samples.reserve(1000000);
          char received[65536];
          while (Clock::now() < finish) {
            for (size_t i = 0; i < connections.size(); ++i)
              descriptors[i] = pollfd{connections[i].fd, static_cast<short>(connections[i].sent < payload.size() ? POLLOUT : POLLIN), 0};
            if (::poll(descriptors.data(), descriptors.size(), 20) < 0) {
              if (errno == EINTR) continue;
              throw std::runtime_error("poll failed");
            }
            for (size_t i = 0; i < connections.size(); ++i) {
              auto& connection = connections[i];
              const short events = descriptors[i].revents;
              if (!events || connection.fd < 0) continue;
              bool failed = (events & (POLLERR | POLLNVAL)) != 0;
              if (!failed && (events & POLLOUT)) {
                const ssize_t sent = ::send(connection.fd, payload.data() + connection.sent,
                                             payload.size() - connection.sent, MSG_NOSIGNAL);
                if (sent > 0) connection.sent += static_cast<size_t>(sent);
                else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) failed = true;
              }
              if (!failed && (events & POLLIN)) {
                const size_t remaining = payload.size() - connection.received;
                const ssize_t got = ::recv(connection.fd, received, std::min(remaining, sizeof received), 0);
                if (got > 0) {
                  if (std::memcmp(received, payload.data() + connection.received, static_cast<size_t>(got))) failed = true;
                  connection.received += static_cast<size_t>(got);
                  if (connection.received == payload.size()) {
                    const auto now = Clock::now();
                    if (connection.began >= measuredStart && now <= finish) {
                      const double latency = std::chrono::duration<double, std::micro>(now - connection.began).count();
                      ++result.requests;
                      result.totalLatency += latency;
                      if (result.samples.size() < 1000000) result.samples.push_back(latency);
                      else {
                        std::uniform_int_distribution<uint64_t> pick(0, result.requests - 1);
                        const uint64_t index = pick(random);
                        if (index < result.samples.size()) result.samples[index] = latency;
                      }
                    }
                    connection.sent = connection.received = 0;
                    connection.began = now;
                  }
                } else if (got == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) failed = true;
              }
              if (events & POLLHUP) failed = true;
              if (failed) { ++result.errors; ::close(connection.fd); connection.fd = -1; }
            }
          }
        } catch (const std::exception& error) {
          result.failure = error.what();
          ++result.errors;
          if (!announced) announce();
        }
      });
    }
    {
      std::unique_lock<std::mutex> lock(readyMutex);
      readyCondition.wait(lock, [&] { return ready == threads; });
    }
    startPromise.set_value(Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(warmup)));
    for (auto& worker : workers) worker.join();
    uint64_t completed = 0, errors = 0;
    double totalLatency = 0;
    std::vector<std::pair<double, double>> samples;
    for (const auto& result : results) {
      completed += result.requests;
      errors += result.errors;
      totalLatency += result.totalLatency;
      const double weight = result.samples.empty() ? 0 : static_cast<double>(result.requests) / result.samples.size();
      for (double sample : result.samples) samples.emplace_back(sample, weight);
      if (!result.failure.empty()) std::cerr << result.failure << '\n';
    }
    std::sort(samples.begin(), samples.end());
    const auto percentile = [&](double fraction) {
      double accumulated = 0;
      for (const auto& sample : samples) {
        accumulated += sample.second;
        if (accumulated >= fraction * completed) return sample.first;
      }
      return samples.empty() ? 0.0 : samples.back().first;
    };
    std::cout << std::fixed << std::setprecision(2)
              << "requests=" << completed << " errors=" << errors << " seconds=" << seconds
              << " requests_per_sec=" << completed / seconds
              << " echo_mib_per_sec=" << completed * static_cast<double>(bytes) / seconds / 1048576.0
              << " bidirectional_mib_per_sec=" << completed * static_cast<double>(bytes) * 2 / seconds / 1048576.0
              << " mean_rtt_us=" << (completed ? totalLatency / completed : 0)
              << " p50_rtt_us=" << percentile(0.50) << " p95_rtt_us=" << percentile(0.95)
              << " p99_rtt_us=" << percentile(0.99) << " latency_samples=" << samples.size() << '\n';
    return errors || completed == 0 ? 2 : 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
