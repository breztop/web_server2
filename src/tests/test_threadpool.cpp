#define BOOST_TEST_MODULE ThreadPoolTest
#include <boost/test/included/unit_test.hpp>
#include "breutil/thread_pool.hpp"
#include <atomic>
#include <thread>
#include <chrono>

BOOST_AUTO_TEST_SUITE(ThreadPoolTestSuite)

BOOST_AUTO_TEST_CASE(test_basic_submit) {
    bre::ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    
    for (int i = 0; i < 100; ++i) {
        pool.Enqueue([&counter]() {
            ++counter;
        });
    }
    
    pool.WaitAll();
    BOOST_CHECK_EQUAL(counter.load(), 100);
}

BOOST_AUTO_TEST_CASE(test_future_result) {
    bre::ThreadPool pool(2);
    
    auto future = pool.Enqueue([]() { return 42; });
    BOOST_CHECK_EQUAL(future.get(), 42);
}

BOOST_AUTO_TEST_CASE(test_multiple_futures) {
    bre::ThreadPool pool(4);
    
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.Enqueue([i]() { return i * i; }));
    }
    
    for (int i = 0; i < 10; ++i) {
        BOOST_CHECK_EQUAL(futures[i].get(), i * i);
    }
}

BOOST_AUTO_TEST_CASE(test_exception_handling) {
    bre::ThreadPool pool(2);
    
    auto future = pool.Enqueue([]() -> int {
        throw std::runtime_error("test exception");
    });
    
    BOOST_CHECK_THROW(future.get(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_thread_count) {
    bre::ThreadPool pool(8);
    BOOST_CHECK_EQUAL(pool.GetThreadCount(), 8);
}

BOOST_AUTO_TEST_CASE(test_wait_all) {
    bre::ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    
    for (int i = 0; i < 100; ++i) {
        pool.Enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++counter;
        });
    }
    
    pool.WaitAll();
    BOOST_CHECK_EQUAL(counter.load(), 100);
    BOOST_CHECK_EQUAL(pool.GetQueueSize(), 0);
}

BOOST_AUTO_TEST_CASE(test_concurrent_submit) {
    bre::ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 100; ++i) {
                pool.Enqueue([&counter]() {
                    ++counter;
                });
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    pool.WaitAll();
    BOOST_CHECK_EQUAL(counter.load(), 1000);
}

BOOST_AUTO_TEST_CASE(test_queue_size) {
    bre::ThreadPool pool(1);  // 只用1个线程，方便测试队列
    
    std::atomic<bool> hold{true};
    
    // 提交一个长时间运行的任务
    pool.Enqueue([&hold]() {
        while (hold.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // 等待任务开始执行
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 提交更多任务到队列
    for (int i = 0; i < 10; ++i) {
        pool.Enqueue([]() {});
    }
    
    BOOST_CHECK(pool.GetQueueSize() > 0);
    
    // 释放第一个任务
    hold.store(false);
    pool.WaitAll();
    BOOST_CHECK_EQUAL(pool.GetQueueSize(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
