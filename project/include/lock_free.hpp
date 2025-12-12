/* Ex 5 Lock free implementation*/

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

namespace lock_free
{
    using value_t = generics::value_t; // To be changed if one will
    value_t empty_val = generics::empty_val;

    struct Node
    {
        std::atomic<Node *> next;
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

            Node *walker = header;
            while (walker->next != nullptr)
            {
                Node *next_ptr = static_cast<Node *>(walker->next);
                if (next_ptr->value == val)
                {
                    auto to_rtn = next_ptr;
                    next_ptr = static_cast<Node *>(next_ptr->next);
                    size--;
                    return to_rtn;
                }
                walker = walker->next.load();
            }

            return nullptr;
        }
    };

    class Queue : public BaseQueue
    { // FIFO
        std::atomic<Node *> header;
        std::atomic<Node *> tail;

        std::vector<FreeList> freelists;
        std::atomic<int> size;
        omp_lock_t header_lock;
        omp_lock_t tail_lock;

    public:
        int header_tail_condition = 0;
        int null_header_cnt = 0;
        Queue()
        {
            Node *h = new Node;
            h->next = nullptr;
            h->value = empty_val;

            header.store(h, std::memory_order_relaxed);
            tail.store(h, std::memory_order_relaxed);

            size.store(0, std::memory_order_relaxed);

            int n_threads = omp_get_max_threads();
            freelists.resize(n_threads);
        }

        ~Queue()
        {
            Node *walker = header.load(std::memory_order_relaxed);
            while (walker != nullptr)
            {
                Node *current = walker;
                walker = walker->next;
                delete current;
            }
        };

        bool push(value_t val) override
        {
            int tid = omp_get_thread_num();
            Node *n = freelists[tid].get(val);
            if (n == nullptr)
            {
                n = new Node;
                n->value = val;
            }
            n->next = nullptr;

            while (true)
            {
                Node *expected = nullptr;
                Node *last = tail.load();
                Node *next = last->next;

                if (next == nullptr)
                {
                    if (std::atomic_compare_exchange_weak(
                            &(last->next), &expected, n))
                    {
                        tail.compare_exchange_weak(
                            last, n, std::memory_order_release, std::memory_order_relaxed);
                        size.fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }
                }
                else
                {
                    tail.compare_exchange_weak(last, next, std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }

        value_t pop() override
        {
            int tid = omp_get_thread_num();

            while (true)
            {

                Node *first = header.load(std::memory_order_acquire);
                Node *last_ptr = tail.load(std::memory_order_acquire);
                Node *next = first->next.load(std::memory_order_acquire);

                if (first == last_ptr)
                {
                    if (next == nullptr)
                    {
                        return empty_val; // queue is empty
                    }
                    tail.compare_exchange_weak(last_ptr, next,
                                               std::memory_order_release,
                                               std::memory_order_relaxed);
                }
                else
                {
                    value_t val = next->value;
                    if (header.compare_exchange_weak(first, next, std::memory_order_relaxed))
                    {
                        size.fetch_sub(1, std::memory_order_relaxed);
                        return val;
                    }
                }
            }
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