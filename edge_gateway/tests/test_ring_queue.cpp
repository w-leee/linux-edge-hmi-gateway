#include <gtest/gtest.h>
#include "RingQueue.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

// 测试 1：基础的单线程入队和出队
TEST(RingQueueTest, BasicPushPop) {
    RingQueue<int> q(5);
    
    EXPECT_EQ(q.size(), 0);
    
    q.push(10);
    q.push(20);
    EXPECT_EQ(q.size(), 2);
    
    EXPECT_EQ(q.pop(), 10);
    EXPECT_EQ(q.pop(), 20);
    EXPECT_EQ(q.size(), 0);
}

// 测试 2：环形覆盖机制（测试指针归零绕圈）
TEST(RingQueueTest, CircularWrap) {
    RingQueue<int> q(3);
    
    q.push(1); q.push(2); q.push(3);
    EXPECT_EQ(q.pop(), 1); // 腾出一个位置
    
    q.push(4); // 写入位置应该绕回到索引 0
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
    EXPECT_EQ(q.pop(), 4);
}

// 测试 3：多线程并发极限压测（1个生产者，1个消费者）
TEST(RingQueueTest, ConcurrentSingleProducerConsumer) {
    RingQueue<int> q(10);
    const int total_items = 10000;
    std::atomic<int> sum{0};

    // 生产者线程
    std::thread producer([&]() {
        for (int i = 1; i <= total_items; ++i) {
            q.push(i);
        }
    });

    // 消费者线程
    std::thread consumer([&]() {
        for (int i = 0; i < total_items; ++i) {
            sum += q.pop();
        }
    });

    producer.join();
    consumer.join();

    // 计算 1 到 10000 的累加和，公式：n*(n+1)/2
    int expected_sum = total_items * (total_items + 1) / 2;
    EXPECT_EQ(sum.load(), expected_sum);
}

// 测试 4：多生产者、多消费者并发
TEST(RingQueueTest, ConcurrentMultiProducerMultiConsumer) {
    RingQueue<int> q(5); // 故意把队列搞得很小，逼迫频繁发生锁竞争和唤醒
    const int num_producers = 3;
    const int num_consumers = 3;
    const int items_per_producer = 1000;
    const int total_items = num_producers * items_per_producer;
    
    std::atomic<int> sum{0};
    std::vector<std::thread> threads;

    // 起 3 个生产者
    for (int p = 0; p < num_producers; ++p) {
        threads.emplace_back([&, p]() {
            for (int i = 0; i < items_per_producer; ++i) {
                q.push(1); // 每个生产者塞 1000 个 1
            }
        });
    }

    // 起 3 个消费者
    for (int c = 0; c < num_consumers; ++c) {
        threads.emplace_back([&]() {
            for (int i = 0; i < items_per_producer; ++i) {
                sum += q.pop();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 3 个生产者各塞 1000 个 1，总和应该是 3000
    EXPECT_EQ(sum.load(), total_items);
}
