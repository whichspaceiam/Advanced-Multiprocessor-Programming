// Part of ex 4

/*
The only logical difference with the sequential lock is in the push and pop
implementation. Apart from it of course we have to initialize and destroy the
locks.
*/

#pragma once
#include "base_queue.hpp"
#include "generics.hpp"
#include <atomic>
#include <cassert>
#include <limits>
#include <omp.h>
#include <unordered_set>
#include <vector>

namespace finelock
{
using value_t = generics::value_t; // To be changed if one will
value_t empty_val = generics::empty_val;

struct Node
{
    Node *next;
    value_t value;
};

class FreeList
{
    Node *header = nullptr;
    unsigned int size = 0;

  public:
    FreeList() = default;

    ~FreeList()
    {
        Node *walker = header;
        while (walker != nullptr)
        {
            Node *current = walker;
            walker = walker->next;
            delete current;
        }
    }

    // FreeList(FreeList const &other) = delete;
    // // FreeList(FreeList &&other) = delete;
    // FreeList &operator=(FreeList const &other) = delete;
    // // FreeList &operator=(FreeList &&other) = delete;

    // RULE OF 5 IMPLEMNTATION OF COPY CONSTRUCTORS AND SO ON

    // Copy constructor (deep copy)
    FreeList(const FreeList &other)
    {
        if (!other.header)
        {
            header = nullptr;
            return;
        }

        // Copy first node
        header = new Node;
        header->value = other.header->value;

        Node *cur_this = header;
        Node *cur_other = other.header->next;

        // Copy the rest of the nodes
        while (cur_other)
        {
            Node *n = new Node;
            n->value = cur_other->value;
            n->next = nullptr;

            cur_this->next = n;
            cur_this = n;
            cur_other = cur_other->next;
        }
    }

    // Copy assignment (deep copy)
    FreeList &operator=(const FreeList &other)
    {
        if (this == &other)
            return *this;

        // Delete current list
        Node *walker = header;
        while (walker)
        {
            Node *current = walker;
            walker = walker->next;
            delete current;
        }

        // Copy new list (same as copy ctor)
        if (!other.header)
        {
            header = nullptr;
            return *this;
        }

        header = new Node;
        header->value = other.header->value;

        Node *cur_this = header;
        Node *cur_other = other.header->next;

        while (cur_other)
        {
            Node *n = new Node;
            n->value = cur_other->value;
            n->next = nullptr;

            cur_this->next = n;
            cur_this = n;
            cur_other = cur_other->next;
        }

        return *this;
    }

    // Move constructor
    FreeList(FreeList &&other) noexcept
    {
        header = other.header;
        other.header = nullptr;
    }

    // Move assignment
    FreeList &operator=(FreeList &&other) noexcept
    {
        if (this != &other)
        {
            // delete current
            Node *walker = header;
            while (walker)
            {
                Node *current = walker;
                walker = walker->next;
                delete current;
            }

            header = other.header;
            other.header = nullptr;
        }
        return *this;
    }
    /// END OF COPY CONSTRUCTORS

    void push(Node *n)
    {
        n->next = header;
        header = n;
        size++;
    }

    Node *get(value_t val)
    {
        if (header == nullptr)
            return nullptr;

        if (header->value == val)
        {
            Node *to_rtn = header;
            header = header->next;
            size--;
            return to_rtn;
        }

        auto walker = header;
        while (walker->next != nullptr)
        {
            if (walker->next->value == val)
            {
                auto to_rtn = walker->next;
                walker->next = walker->next->next;
                size--;
                return to_rtn;
            }
            walker = walker->next;
        }

        return nullptr;
    }
};

class Queue : public BaseQueue
{ // FIFO
    Node *header;
    Node *tail;

    std::vector<FreeList> freelists;
    std::atomic<int> size;
    omp_lock_t header_lock;
    omp_lock_t tail_lock;

  public:
    int header_tail_condition = 0;
    int null_header_cnt = 0;
    Queue()
    {
        omp_init_lock(&header_lock);
        omp_init_lock(&tail_lock);
        header = new Node;
        header->next = nullptr;
        header->value = empty_val;
        tail = header;
        size = 0;
        int n_threads = omp_get_max_threads();
        freelists.resize(n_threads);
    };

    ~Queue()
    {
        Node *walker = header;
        while (walker != nullptr)
        {
            Node *current = walker;
            walker = walker->next;
            delete current;
        }

        omp_destroy_lock(&header_lock);
        omp_destroy_lock(&tail_lock);
    };

    bool push(value_t val) override
    {
        int tid = omp_get_thread_num();
        omp_set_lock(&tail_lock);
        Node *n = freelists[tid].get(val);
        if (n == nullptr)
        {
            n = new Node;
            n->value = val;
        }
        n->next = nullptr;
        tail->next = n;
        tail = n;
        size++;
        omp_unset_lock(&tail_lock);

        return true;
    }

    value_t pop() override
    {
        int tid = omp_get_thread_num();
        omp_set_lock(&header_lock);
        omp_set_lock(&tail_lock);


        // if (header->next == nullptr)
        // {
        //     omp_unset_lock(&header_lock);
        //     return empty_val;
        // }

        Node *current = header->next;
        value_t val = current->value;

        // bool maybe_last = (current == tail);

        // if (!maybe_last)
        // {
        //     // common case: unlink under header_lock only
        //     header->next = current->next;
        //     // update bookkeeping while still holding header_lock
            // freelists[tid].push(current);
            // --size;
        //     omp_unset_lock(&header_lock);
        //     return val;
        // }

        // // possible last node: acquire tail_lock while holding header_lock
        // omp_set_lock(&tail_lock);

        // // re-check under both locks (another thread may have changed tail)
        // if (current == tail)
        // {
        //     // still last -> update head and tail atomically (w.r.t. push)
        //     header->next = current->next; // should be nullptr
        //     assert(header->next == nullptr);
        //     tail = header;
        //     freelists[tid].push(current);
        //     --size;
        //     omp_unset_lock(&tail_lock);
        //     omp_unset_lock(&header_lock);
        //     return val;
        // }

        // tail changed meanwhile: handle as non-last
        omp_unset_lock(&tail_lock);
        header->next = current->next;
        freelists[tid].push(current);
        --size;
        omp_unset_lock(&header_lock);
        return val;
    }

    int get_size() const override { return size.load(); }

    Node const *get_head() const { return header; }
    Node const *get_tail() const { return tail; }

    Queue(Queue const &other) = delete;
    Queue(Queue const &&other) = delete;

    Queue &operator=(Queue const &other) = delete;
    Queue &operator=(Queue const &&other) = delete;
};
}; // namespace finelock