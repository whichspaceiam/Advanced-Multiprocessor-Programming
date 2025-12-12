#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include "atomic.hpp"

// using namespace as;

// struct alignas(1<<17)
// Node {
//     int value;
// };
// template class as::TaggedPointer<Node>;
using namespace as;
void test_tagged_pointer_basic()
{
    Node n1, n2;
    // Node n2;
    n1.value = 10;
    n2.value = 20;

    as::TaggedPointer<Node> tp(&n1, 42, true);

    // Test getPointer, getVersion, getMark
    Node* ptr;
    uint16_t ver;
    bool mark;
    tp.get(&ptr, &ver, &mark);

    assert(ptr == &n1);
    assert(ver == 42);
    assert(mark == true);

    std::cout << "Basic get test passed\n";

    // Test compareAndSet success
    bool cas_result = tp.compareAndSet(&n1, 42, true, &n2, 55, false);
    assert(cas_result);

    tp.get(&ptr, &ver, &mark);
    assert(ptr == &n2);
    assert(ver == 55);
    assert(mark == false);

    std::cout << "compareAndSet success test passed\n";

    // Test compareAndSet failure (wrong expected pointer)
    cas_result = tp.compareAndSet(&n1, 55, false, &n1, 99, true);
    assert(!cas_result);

    tp.get(&ptr, &ver, &mark);
    assert(ptr == &n2);
    assert(ver == 55);
    assert(mark == false);

    std::cout << "compareAndSet failure test passed\n";
}

void test_tagged_pointer_multithreaded()
{
    as::TaggedPointer<Node> tp(nullptr, 0, false);
    const int nThreads = 4;
    const int nIters = 1000;

    std::vector<Node*> nodes(nThreads * nIters);
    for (auto& p : nodes)
        p = new Node{0};

    auto worker = [&](int id) {
        for (int i = 0; i < nIters; ++i)
        {
            Node* oldPtr;
            uint16_t oldVer;
            bool oldMark;
            tp.get(&oldPtr, &oldVer, &oldMark);

            tp.compareAndSet(oldPtr, oldVer, oldMark, nodes[id * nIters + i], oldVer + 1, !oldMark);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < nThreads; ++i)
        threads.emplace_back(worker, i);

    for (auto& t : threads) t.join();

    Node* finalPtr;
    uint16_t finalVer;
    bool finalMark;
    tp.get(&finalPtr, &finalVer, &finalMark);

    std::cout << "Multithreaded test finished: final version = " 
              << finalVer << ", mark = " << finalMark << "\n";

    for (auto p : nodes)
        delete p;
}

int main()
{
    test_tagged_pointer_basic();
    test_tagged_pointer_multithreaded();

    std::cout << "All tests passed\n";
    return 0;
}


