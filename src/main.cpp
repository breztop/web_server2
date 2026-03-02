#include "server/WebServer.hpp"
#include <iostream>
#include <csignal>
#include <memory>

// 全局WebServer指针用于信号处理
std::unique_ptr<bre::WebServer> g_server;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_server) {
        g_server->Stop();
    }
}

int main() {
    try {
        // 注册信号处理器
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);
        
#ifndef _WIN32
        // Linux下忽略SIGPIPE（防止socket写入已关闭的连接时崩溃）
        std::signal(SIGPIPE, SIG_IGN);
#endif

        std::cout << "==================================" << std::endl;
        std::cout << "  Bre WebServer v2.0" << std::endl;
        std::cout << "  ASIO + PostgreSQL + C++20" << std::endl;
        std::cout << "  Architecture: Main Reactor + IO Pool" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << std::endl;

        // 创建并启动服务器
        g_server = std::make_unique<bre::WebServer>();
        g_server->Start();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }

    return 0;
} 