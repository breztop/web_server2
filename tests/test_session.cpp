#define BOOST_TEST_MODULE SessionTest
#include <boost/asio.hpp>
#include <boost/test/included/unit_test.hpp>
#include <memory>
#include <string>

#include "breutil/buffer.hpp"
#include "server/Session.hpp"

BOOST_AUTO_TEST_SUITE(SessionTestSuite)

BOOST_AUTO_TEST_CASE(test_session_creation) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);

    bre::Session session(std::move(socket), "./public");

    BOOST_CHECK_EQUAL(session.GetSessionId(), 1ULL);
}

BOOST_AUTO_TEST_CASE(test_session_id_increments) {
    boost::asio::io_context io_context1;
    boost::asio::ip::tcp::socket socket1(io_context1);
    auto session1 = std::make_shared<bre::Session>(std::move(socket1), "./public");
    uint64_t id1 = session1->GetSessionId();

    boost::asio::io_context io_context2;
    boost::asio::ip::tcp::socket socket2(io_context2);
    auto session2 = std::make_shared<bre::Session>(std::move(socket2), "./public");
    uint64_t id2 = session2->GetSessionId();

    BOOST_CHECK(id2 > id1);
}

BOOST_AUTO_TEST_CASE(test_session_remote_address) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);

    bre::Session session(std::move(socket), "./public");

    std::string addr = session.GetRemoteAddress();
    BOOST_CHECK_EQUAL(addr, "unknown");
}

BOOST_AUTO_TEST_SUITE_END()
