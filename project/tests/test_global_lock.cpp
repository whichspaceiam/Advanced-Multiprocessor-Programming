#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "lock_guard.hpp"   // your global_lock::ConcurrentQueue

using global_lock::ConcurrentQueue;
using global_lock::value_t;

int main() {
    
    // ------------------------------------------------------------
    std::cout << "TEST 0: Empty val assertion\n";
    // ------------------------------------------------------------
    {
        for(int i =1; i<1'000'000; i++){
            assert(seq::empty_val != i);
        }
    }

    // ------------------------------------------------------------
    std::cout << "TEST 1: Single-thread push/pop\n";
    // ------------------------------------------------------------
    {
        ConcurrentQueue cq;
        cq.push(10);
        cq.push(20);
        cq.push(30);

        assert(cq.pop() == 10);
        assert(cq.pop() == 20);
        assert(cq.pop() == 30);
        assert(cq.pop() == seq::empty_val);
    }


    // ------------------------------------------------------------
    std::cout << "TEST 2: Two threads doing only push (multiple producers)\n";
    // ------------------------------------------------------------
    {
        ConcurrentQueue cq;

        auto producer = [&](int start) {
            for (int i = 0; i < 1000; i++)
                cq.push(start + i);
        };

        std::thread t1(producer, 10000);
        std::thread t2(producer, 20000);

        t1.join();
        t2.join();

        int count = 0;
        while (cq.pop() != seq::empty_val)
            count++;

        // Expect exactly 2000 pushes
        assert(count == 2000);
    }


    // ------------------------------------------------------------
    std::cout << "TEST 3: Two threads doing only pop (multiple consumers)\n";
    // ------------------------------------------------------------
    {
        ConcurrentQueue cq;

        int total_sum = 0;
        for (int i = 0; i < 2000; i++){
            cq.push(i);
            total_sum+=i;
        }


        std::cout<< "Size: "<<cq.get_size()<<"\n";

        int c1 = 0;
        int c2 = 0;

        int s1 = 0;
        int s2 = 0;

        auto consumer = [&](int& counter, int& sum) {
            
            while (true) {
                value_t v = cq.pop();
                if (v == seq::empty_val) break;
                sum+=v;
                counter++;
            }
        };

        std::thread t1(consumer, std::ref(c1), std::ref(s1));
        std::thread t2(consumer, std::ref(c2), std::ref(s2));

        t1.join();
        t2.join();

        std::cout<<"c1, c2, Size:  "<< c1<<", "<< c2<<", "<< cq.get_size()<<std::endl;
        std::cout<<"s1, s2  "<< s1<<", "<<s2<<std::endl;

        assert(total_sum==s1+s2);
        assert(c1 + c2 == 2000);
    }


    // ------------------------------------------------------------
    std::cout << "TEST 4: Producer-consumer mix (2 producers + 2 consumers)\n";
    // ------------------------------------------------------------
    {
        ConcurrentQueue cq;

        auto producer = [&](int start) {
            for (int i = 0; i < 1000; i++)
                cq.push(start + i);
        };

        int c1 = 0, c2 = 0;
        auto consumer = [&](int& counter) {
            while (true) {
                value_t v = cq.pop();
                if (v == seq::empty_val) break;
                counter++;
            }
        };

        std::thread p1(producer, 0);
        std::thread p2(producer, 10000);

        // give producers a tiny head-start so consumers have something to pop
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        std::thread cA(consumer, std::ref(c1));
        std::thread cB(consumer, std::ref(c2));

        p1.join();
        p2.join();

        // signal consumers by emptying the queue
        // pop until empty_val repeatedly
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        cA.join();
        cB.join();

        assert(c1 + c2 == 2000);
    }


    // ------------------------------------------------------------
    std::cout << "TEST 5: High contention stress test (many threads pushing/popping)\n";
    // ------------------------------------------------------------
    {
        ConcurrentQueue cq;

        auto producer = [&]() {
            for (int i = 0; i < 2000; i++)
                cq.push(i);
        };

        int consumed_total = 0;
        std::mutex count_m;

        auto consumer = [&]() {
            while (true) {
                value_t v = cq.pop();
                if (v == seq::empty_val) break;
                std::lock_guard<std::mutex> lk(count_m);
                consumed_total++;
            }
        };

        std::vector<std::thread> threads;

        for (int i = 0; i < 4; i++)
            threads.emplace_back(producer);
        for (int i = 0; i < 4; i++)
            threads.emplace_back(consumer);

        for (auto& t : threads) t.join();

        // 4 producers * 2000 pushes = 8000 total
        assert(consumed_total == 8000);
    }

    std::cout << "All concurrent queue tests passed.\n";

    return 0;
}
