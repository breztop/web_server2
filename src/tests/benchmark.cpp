#include <benchmark/benchmark.h>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// 平台相关头文件
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define ssize_t int
#endif

// 全局配置 (可以通过命令行参数传递，这里简化为常量)
static const std::string kHost = "127.0.0.1";
static const uint16_t kPort = 8080;
static const std::string kRequestPath = "/index.html";

// 辅助函数：创建 HTTP 请求字符串
static std::string BuildRequest(bool keep_alive) {
    std::string req = "GET " + kRequestPath + " HTTP/1.1\r\n";
    req += "Host: " + kHost + "\r\n";
    req += keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n";
    req += "\r\n";
    return req;
}

static const std::string kKeepAliveReq = BuildRequest(true);
static const std::string kCloseReq = BuildRequest(false);

// 简单的 Socket 包装器，用于 Benchmark 上下文
struct ClientSocket {
    SOCKET fd = INVALID_SOCKET;

    ClientSocket() {
#ifdef _WIN32
        // 注意：实际生产中 WSADATA 应只初始化一次，这里为了简单放在构造中，
        // 但在 benchmark 多次运行中可能重复调用，生产环境建议单例管理。
        static bool ws_initialized = false;
        if (!ws_initialized) {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            ws_initialized = true;
        }
#endif
    }

    ~ClientSocket() { Close(); }

    bool Connect() {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCKET) return false;

        struct sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(kPort);

        if (inet_pton(AF_INET, kHost.c_str(), &serv_addr.sin_addr) <= 0) {
            Close();
            return false;
        }

        if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            Close();
            return false;
        }
        return true;
    }

    bool SendAndRecv(const std::string& request) {
        if (fd == INVALID_SOCKET) return false;

        ssize_t sent = send(fd, request.c_str(), request.size(), 0);
        if (sent != static_cast<ssize_t>(request.size())) return false;

        char buffer[4096];
        ssize_t received = recv(fd, buffer, sizeof(buffer) - 1, 0);
        return received > 0;
    }

    void Close() {
        if (fd != INVALID_SOCKET) {
            closesocket(fd);
            fd = INVALID_SOCKET;
        }
    }
};

// -----------------------------------------------------------------------------
// Benchmark 1: 单连接顺序请求 (模拟原 RunSimpleTest，但测量单次请求耗时)
// 注意：这里每次迭代都新建连接，开销较大，主要测试连接建立 + 简单请求的总耗时
// -----------------------------------------------------------------------------
static void BM_SingleRequest_NewConnection(benchmark::State& state) {
    for (auto _ : state) {
        ClientSocket client;
        if (client.Connect()) {
            if (!client.SendAndRecv(kCloseReq)) {
                state.SkipWithError("Failed to send/recv");
            }
        } else {
            state.SkipWithError("Failed to connect");
        }
    }
}
BENCHMARK(BM_SingleRequest_NewConnection)->Unit(benchmark::kMillisecond);

// -----------------------------------------------------------------------------
// Benchmark 2: Keep-Alive 吞吐量 (模拟原 RunKeepAliveTest)
// 在 Setup/Teardown 中维护长连接，循环内只进行请求/响应，排除握手开销
// -----------------------------------------------------------------------------
static void BM_KeepAlive_Throughput(benchmark::State& state) {
    ClientSocket client;
    // 预热：建立连接
    if (!client.Connect()) {
        state.SkipWithError("Failed to connect initial");
        return;
    }

    for (auto _ : state) {
        if (!client.SendAndRecv(kKeepAliveReq)) {
            state.SkipWithError("Failed to send/recv in loop");
            break;
        }
    }

    // 清理
    client.Close();
}
BENCHMARK(BM_KeepAlive_Throughput)->Unit(benchmark::kMicrosecond);

// -----------------------------------------------------------------------------
// Benchmark 3: 并发压力测试 (模拟原 RunConcurrentTest)
// Google Benchmark 默认单线程运行一个用例。要测试并发，需使用 MultiThreaded 模式
// 或者使用 --benchmark_min_threads / --benchmark_max_threads 命令行参数
// 这里演示如何在代码中固定线程数 (例如 10 个并发线程)
// -----------------------------------------------------------------------------
static void BM_Concurrent_Requests(benchmark::State& state) {
    // 每个线程拥有自己的连接
    ClientSocket client;
    if (!client.Connect()) {
        state.SkipWithError("Failed to connect");
        return;
    }

    for (auto _ : state) {
        // 在并发模式下，state.iterations() 会被分摊到各个线程
        if (!client.SendAndRecv(kKeepAliveReq)) {
            state.SkipWithError("Request failed");
            break;
        }
    }

    client.Close();
}
// 设置并发线程数为 10 (对应原代码的 10 threads)
BENCHMARK(BM_Concurrent_Requests)->Threads(10)->Unit(benchmark::kMicrosecond);

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
BENCHMARK_MAIN();