#include <cassert>
#include "sequential.hpp"
#include <iostream>

using namespace seq;    // If your code is inside namespace seq

// Helper to check internal invariants:
static void check_tail_correctness(const Queue& q) {
    // Ensure that if header->next == nullptr, tail == header
    const Node* h = q.get_head();
    const Node* t = q.get_tail();

    if (h->next == nullptr) {
        assert(t == h);
    }
}
int main() {

    std::cout << "TEST 1: New queue is empty, pop returns empty_val"<<std::endl;
    {
        Queue q;
        assert(q.pop() == empty_val);
        check_tail_correctness(q);
    }

    std::cout << "TEST Single push"<<std::endl;
    {
        Queue q;
        Node const * head_org = q.get_head();
        q.push(10);
        Node const * tail = q.get_tail();
        Node const * head = q.get_head();
        assert(head_org == head);
        assert(head!=tail);
        assert(tail->value==10);
        
    }
    
    std::cout << "TEST 2: Push one value, pop returns it, queue becomes empty"<<std::endl;
    {
        Queue q;
        
        Node const * tail_org = q.get_tail();
        Node const * head_org = q.get_head();
        
        assert(tail_org == head_org);
        
        q.push(10);
        Node const * head_1 = q.get_head();
        Node const * tail_1 = q.get_tail();
        assert(head_1->next==tail_1);
        


        value_t v = q.pop();
        Node const * tail = q.get_tail();
        Node const * head = q.get_head();

        assert(head == head_org);

        if(tail != head){
            std::cout<<" !! TAil is different than head !!"<<std::endl;
        }

        assert(std::abs(v-10)<10*std::numeric_limits<double>::epsilon());
        assert(q.pop() == empty_val);
        check_tail_correctness(q);
    }

    std::cout << "TEST 3: Push two, pop in FIFO order\n";
    {
        Queue q;
        q.push(1);
        q.push(2);

        assert(q.pop() == 1);
        assert(q.pop() == 2);
        assert(q.pop() == empty_val);
        check_tail_correctness(q);
    }

    std::cout << "TEST 4: Push 3, pop 2, push 2, check reuse and tail correctness"<<std::endl;
    {
        Queue q;
        q.push(5);
        q.push(6);
        q.push(7);

        assert(q.pop() == 5);
        assert(q.pop() == 6);

        q.push(8);
        q.push(9);

        assert(q.pop() == 7);
        assert(q.pop() == 8);
        assert(q.pop() == 9);
        assert(q.pop() == empty_val);

        check_tail_correctness(q);
    }

    std::cout << "TEST 5: Pop until empty, then push again, ensure reset works"<<std::endl;
    {
        Queue q;
        q.push(4);
        q.push(5);

        assert(q.pop() == 4);
        assert(q.pop() == 5);
        assert(q.pop() == empty_val);

        q.push(99);
        assert(q.pop() == 99);
        assert(q.pop() == empty_val);

        check_tail_correctness(q);
    }

    std::cout << "TEST 6: Many alternating push/pop operations"<<std::endl;
    {
        Queue q;
        for (int i = 0; i < 1000; i++) {
            q.push(i);
            assert(q.pop() == i);
        }

        assert(q.pop() == empty_val);
        check_tail_correctness(q);
    }

    std::cout << "TEST 7: Push multiple identical values – check reuse logic"<<std::endl;
    {
        Queue q;
        q.push(1);
        q.push(1);
        q.push(1);

        assert(q.pop() == 1);
        assert(q.pop() == 1);
        assert(q.pop() == 1);
        assert(q.pop() == empty_val);

        check_tail_correctness(q);
    }

    std::cout << "TEST 8: Stress test – push 100, pop 100"<<std::endl;
    {
        Queue q;
        for(int i = 0; i < 100; i++) q.push(i);
        for(int i = 0; i < 100; i++) assert(q.pop() == i);
        assert(q.pop() == empty_val);

        check_tail_correctness(q);
    }

    std::cout << "TEST 9: Reuse test – freelist nodes must not corrupt queue"<<std::endl;
    {
        Queue q;

        q.push(10);
        q.push(20);
        assert(q.pop() == 10);
        assert(q.pop() == 20);

        assert(q.pop() == empty_val);

        q.push(30);
        q.push(40);

        assert(q.pop() == 30);
        assert(q.pop() == 40);
        assert(q.pop() == empty_val);

        check_tail_correctness(q);
    }

    std::cout << "TEST 10: Mix identical and distinct values"<<std::endl;
    {
        Queue q;

        q.push(5);
        q.push(7);
        q.push(5);

        assert(q.pop() == 5);
        assert(q.pop() == 7);
        assert(q.pop() == 5);
        assert(q.pop() == empty_val);

        check_tail_correctness(q);
    }

    return 0;
}
