#include <iostream>
#include <csignal>
#include <memory>
#include <thread>

#include "boost/asio.hpp"
namespace asio = boost::asio;

#include "breutil/net/asio_io_context_pool.hpp"
#include "server/WebServer.hpp"


int main() {
    try {
        bre::WebServer webServer;


        auto SignalHandler = [&webServer](int signal) {
            std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
            webServer.Stop();
        };
        

        asio::signal_set signals(bre::AsioIOContextPool::Instance()->GetIOContext(), SIGINT, SIGTERM);
        signals.async_wait([&SignalHandler](const boost::system::error_code& error, int signal) {
            if (!error) {
                SignalHandler(signal);
            } else {
                std::cerr << "Signal wait error: " << error.message() << std::endl;
            }
        });

        
        std::cout << "==================================\n"
                << "  Bre WebServer v2.0\n"
                << "==================================\n";

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