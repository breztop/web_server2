#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include "boost/asio.hpp"
namespace asio = boost::asio;

#include "breutil/generate_build_info.hpp"
#include "breutil/net/asio_io_context_pool.hpp"
#include "server/WebServer.hpp"

void print_build_info() {
    std::cout << "=====================================\n"
              << bre::GenerateBuildInfo::GetBuildInfo()
              << "\n=====================================\n";
#ifdef HAVE_POSTGRESQL
    std::cout << "\nPostgreSQL support: ENABLED\n";
#else
#pragma message("PostgreSQL support disabled")
    std::cout << "\nPostgreSQL support: DISABLED\n";
#endif
}


int main() {
    print_build_info();

    try {
        bre::WebServer webServer;


        auto SignalHandler = [&webServer](int signal) {
            std::cout << "\nReceived signal " << signal << ", shutting down gracefully..."
                      << std::endl;
            webServer.Stop();
        };


        asio::signal_set signals(bre::AsioIOContextPool::Instance()->GetIOContext(), SIGINT,
                                 SIGTERM);
        signals.async_wait([&SignalHandler](const boost::system::error_code& error, int signal) {
            if (!error) {
                SignalHandler(signal);
            } else {
                std::cerr << "Signal wait error: " << error.message() << std::endl;
            }
        });


        std::cout << "\n========================================" << std::endl;
        std::cout << "  Bre WebServer 2.0 Starting..." << std::endl;
        std::cout << "========================================" << std::endl;


        webServer.Start();

        std::cout << "Server stopped. Goodbye!" << std::endl;

        bre::AsioIOContextPool::Instance()->Stop();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }

    return 0;
}