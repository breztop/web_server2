#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <cstring>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

/**
 * @brief WebServer性能基准测试
 * 
 * 测试场景:
 * 1. 单连接吞吐量
 * 2. 多连接并发
 * 3. Keep-Alive性能
 * 4. 静态文件服务性能
 */

class BenchmarkClient {
public:
    BenchmarkClient(const std::string& host, uint16_t port)
        : _host(host), _port(port), _sockfd(-1) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~BenchmarkClient() {
        Close();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool Connect() {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0) {
            return false;
        }

        struct sockaddr_in serv_addr;
        std::memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(_port);
        
        if (inet_pton(AF_INET, _host.c_str(), &serv_addr.sin_addr) <= 0) {
            Close();
            return false;
        }

        if (connect(_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            Close();
            return false;
        }

        return true;
    }

    bool SendRequest(const std::string& path, bool keepAlive = false) {
        std::string request = "GET " + path + " HTTP/1.1\r\n";
        request += "Host: " + _host + "\r\n";
        if (keepAlive) {
            request += "Connection: keep-alive\r\n";
        } else {
            request += "Connection: close\r\n";
        }
        request += "\r\n";

        ssize_t sent = send(_sockfd, request.c_str(), request.size(), 0);
        return sent == static_cast<ssize_t>(request.size());
    }

    bool ReceiveResponse() {
        char buffer[4096];
        ssize_t received = recv(_sockfd, buffer, sizeof(buffer) - 1, 0);
        return received > 0;
    }

    void Close() {
        if (_sockfd >= 0) {
#ifdef _WIN32
            closesocket(_sockfd);
#else
            close(_sockfd);
#endif
            _sockfd = -1;
        }
    }

private:
    std::string _host;
    uint16_t _port;
    int _sockfd;
};

class Benchmark {
public:
    struct Result {
        size_t totalRequests = 0;
        size_t successRequests = 0;
        size_t failedRequests = 0;
        double elapsedSeconds = 0.0;
        double qps = 0.0;
        double avgLatencyMs = 0.0;
    };

    static Result RunSimpleTest(const std::string& host, uint16_t port, 
                                 size_t numRequests) {
        std::cout << "\n=== Simple Test ===" << std::endl;
        std::cout << "Sending " << numRequests << " requests..." << std::endl;

        Result result;
        result.totalRequests = numRequests;

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < numRequests; ++i) {
            BenchmarkClient client(host, port);
            if (client.Connect()) {
                if (client.SendRequest("/index.html") && client.ReceiveResponse()) {
                    ++result.successRequests;
                } else {
                    ++result.failedRequests;
                }
                client.Close();
            } else {
                ++result.failedRequests;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.elapsedSeconds = std::chrono::duration<double>(end - start).count();
        result.qps = result.successRequests / result.elapsedSeconds;
        result.avgLatencyMs = (result.elapsedSeconds * 1000.0) / numRequests;

        return result;
    }

    static Result RunConcurrentTest(const std::string& host, uint16_t port,
                                    size_t numThreads, size_t requestsPerThread) {
        std::cout << "\n=== Concurrent Test ===" << std::endl;
        std::cout << "Threads: " << numThreads << std::endl;
        std::cout << "Requests per thread: " << requestsPerThread << std::endl;

        Result result;
        result.totalRequests = numThreads * requestsPerThread;

        std::atomic<size_t> successCount{0};
        std::atomic<size_t> failCount{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (size_t t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, host, port, requestsPerThread]() {
                for (size_t i = 0; i < requestsPerThread; ++i) {
                    BenchmarkClient client(host, port);
                    if (client.Connect()) {
                        if (client.SendRequest("/index.html") && client.ReceiveResponse()) {
                            ++successCount;
                        } else {
                            ++failCount;
                        }
                        client.Close();
                    } else {
                        ++failCount;
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = std::chrono::high_resolution_clock::now();

        result.successRequests = successCount.load();
        result.failedRequests = failCount.load();
        result.elapsedSeconds = std::chrono::duration<double>(end - start).count();
        result.qps = result.successRequests / result.elapsedSeconds;
        result.avgLatencyMs = (result.elapsedSeconds * 1000.0) / result.totalRequests;

        return result;
    }

    static Result RunKeepAliveTest(const std::string& host, uint16_t port,
                                   size_t numConnections, size_t requestsPerConnection) {
        std::cout << "\n=== Keep-Alive Test ===" << std::endl;
        std::cout << "Connections: " << numConnections << std::endl;
        std::cout << "Requests per connection: " << requestsPerConnection << std::endl;

        Result result;
        result.totalRequests = numConnections * requestsPerConnection;

        std::atomic<size_t> successCount{0};
        std::atomic<size_t> failCount{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (size_t c = 0; c < numConnections; ++c) {
            threads.emplace_back([&, host, port, requestsPerConnection]() {
                BenchmarkClient client(host, port);
                if (client.Connect()) {
                    for (size_t i = 0; i < requestsPerConnection; ++i) {
                        bool isLast = (i == requestsPerConnection - 1);
                        if (client.SendRequest("/index.html", !isLast) && 
                            client.ReceiveResponse()) {
                            ++successCount;
                        } else {
                            ++failCount;
                            break;
                        }
                    }
                    client.Close();
                } else {
                    failCount += requestsPerConnection;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = std::chrono::high_resolution_clock::now();

        result.successRequests = successCount.load();
        result.failedRequests = failCount.load();
        result.elapsedSeconds = std::chrono::duration<double>(end - start).count();
        result.qps = result.successRequests / result.elapsedSeconds;
        result.avgLatencyMs = (result.elapsedSeconds * 1000.0) / result.totalRequests;

        return result;
    }

    static void PrintResult(const std::string& testName, const Result& result) {
        std::cout << "\n--- " << testName << " Results ---" << std::endl;
        std::cout << "Total Requests:   " << result.totalRequests << std::endl;
        std::cout << "Successful:       " << result.successRequests << std::endl;
        std::cout << "Failed:           " << result.failedRequests << std::endl;
        std::cout << "Elapsed Time:     " << result.elapsedSeconds << " seconds" << std::endl;
        std::cout << "QPS:              " << result.qps << " req/s" << std::endl;
        std::cout << "Avg Latency:      " << result.avgLatencyMs << " ms" << std::endl;
        std::cout << "Success Rate:     " 
                 << (result.successRequests * 100.0 / result.totalRequests) << "%" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    uint16_t port = 8080;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = static_cast<uint16_t>(std::stoi(argv[2]));
    }

    std::cout << "============================================" << std::endl;
    std::cout << "  WebServer Benchmark Test" << std::endl;
    std::cout << "  Target: " << host << ":" << port << std::endl;
    std::cout << "============================================" << std::endl;

    // 等待用户确认服务器已启动
    std::cout << "\nMake sure the server is running on " << host << ":" << port << std::endl;
    std::cout << "Press Enter to start benchmark..." << std::endl;
    std::cin.get();

    // 测试1: 简单顺序请求
    auto result1 = Benchmark::RunSimpleTest(host, port, 100);
    Benchmark::PrintResult("Simple Test", result1);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 测试2: 并发请求
    auto result2 = Benchmark::RunConcurrentTest(host, port, 10, 100);
    Benchmark::PrintResult("Concurrent Test", result2);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 测试3: Keep-Alive测试
    auto result3 = Benchmark::RunKeepAliveTest(host, port, 10, 100);
    Benchmark::PrintResult("Keep-Alive Test", result3);

    std::cout << "\n============================================" << std::endl;
    std::cout << "  Benchmark Complete!" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}
