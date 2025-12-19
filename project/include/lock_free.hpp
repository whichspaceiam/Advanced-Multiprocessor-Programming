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
    using value_t = generics::value_t;
    value_t empty_val = generics::empty_val;

    struct Node
    {
        std::atomic<Node *> next;
        value_t value;
        
        Node() : next(nullptr), value(empty_val) {}
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
                walker = walker->next.load(std::memory_order_relaxed);
                delete current;
            }
        }

        // Copy constructor (deep copy)
        FreeList(const FreeList &other) : header(nullptr), size(0)
        {
            if (!other.header)
            {
                return;
            }

            // Copy first node
            header = new Node;
            header->value = other.header->value;
            header->next.store(nullptr, std::memory_order_relaxed);

            Node *cur_this = header;
            Node *cur_other = other.header->next.load(std::memory_order_relaxed);

            // Copy the rest of the nodes
            while (cur_other)
            {
                Node *n = new Node;
                n->value = cur_other->value;
                n->next.store(nullptr, std::memory_order_relaxed);

                cur_this->next.store(n, std::memory_order_relaxed);
                cur_this = n;
                cur_other = cur_other->next.load(std::memory_order_relaxed);
                size++;
            }
            size++; // Count the header node
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
                walker = walker->next.load(std::memory_order_relaxed);
                delete current;
            }

            header = nullptr;
            size = 0;

            // Copy new list
            if (!other.header)
            {
                return *this;
            }

            header = new Node;
            header->value = other.header->value;
            header->next.store(nullptr, std::memory_order_relaxed);

            Node *cur_this = header;
            Node *cur_other = other.header->next.load(std::memory_order_relaxed);

            while (cur_other)
            {
                Node *n = new Node;
                n->value = cur_other->value;
                n->next.store(nullptr, std::memory_order_relaxed);

                cur_this->next.store(n, std::memory_order_relaxed);
                cur_this = n;
                cur_other = cur_other->next.load(std::memory_order_relaxed);
                size++;
            }
            size++; // Count the header node

            return *this;
        }

        // Move constructor
        FreeList(FreeList &&other) noexcept : header(other.header), size(other.size)
        {
            other.header = nullptr;
            other.size = 0;
        }

        // Move assignment
        FreeList &operator=(FreeList &&other) noexcept
        {
            if (this != &other)
            {
                // Delete current
                Node *walker = header;
                while (walker)
                {
                    Node *current = walker;
                    walker = walker->next.load(std::memory_order_relaxed);
                    delete current;
                }

                header = other.header;
                size = other.size;
                other.header = nullptr;
                other.size = 0;
            }
            return *this;
        }

        void push(Node *n)
        {
            n->next.store(header, std::memory_order_relaxed);
            header = n;
            size++;
        }

        Node *get(value_t val)
        {
            if (header == nullptr)
                return nullptr;

            // Check if header matches
            if (header->value == val)
            {
                Node *to_rtn = header;
                header = header->next.load(std::memory_order_relaxed);
                size--;
                return to_rtn;
            }

            // Search through the list
            Node *walker = header;
            while (walker->next.load(std::memory_order_relaxed) != nullptr)
            {
                Node *next_ptr = walker->next.load(std::memory_order_relaxed);
                if (next_ptr->value == val)
                {
                    Node *to_rtn = next_ptr;
                    // **FIX: Properly update the link**
                    walker->next.store(next_ptr->next.load(std::memory_order_relaxed), 
                                      std::memory_order_relaxed);
                    size--;
                    return to_rtn;
                }
                walker = next_ptr;
            }

            return nullptr;
        }
        
        unsigned int get_size() const { return size; }
    };

    class Queue : public BaseQueue
    {
        std::atomic<Node *> header;
        std::atomic<Node *> tail;

        std::vector<FreeList> freelists;
        std::atomic<int> size;

    public:
        Queue()
        {
            Node *h = new Node;
            h->next.store(nullptr, std::memory_order_relaxed);
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
                walker = walker->next.load(std::memory_order_relaxed);
                delete current;
            }
        }

        bool push(value_t val) override
        {
            int tid = omp_get_thread_num();
            Node *n = freelists[tid].get(val);
            if (n == nullptr)
            {
                n = new Node;
            }
            n->value = val;
            n->next.store(nullptr, std::memory_order_relaxed);

            while (true)
            {
                Node *last = tail.load(std::memory_order_acquire);
                Node *next = last->next.load(std::memory_order_acquire);

                // Check if tail is still consistent
                if (last == tail.load(std::memory_order_acquire))
                {
                    if (next == nullptr)
                    {
                        // **FIX: expected must be nullptr for each attempt**
                        Node *expected = nullptr;
                        if (last->next.compare_exchange_weak(
                                expected, n,
                                std::memory_order_release,
                                std::memory_order_relaxed))
                        {
                            // Try to swing tail to the new node
                            tail.compare_exchange_strong(
                                last, n,
                                std::memory_order_release,
                                std::memory_order_relaxed);
                            size.fetch_add(1, std::memory_order_relaxed);
                            return true;
                        }
                    }
                    else
                    {
                        // Help other threads by swinging tail forward
                        tail.compare_exchange_weak(
                            last, next,
                            std::memory_order_release,
                            std::memory_order_relaxed);
                    }
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

                // Check if header is still consistent
                if (first == header.load(std::memory_order_acquire))
                {
                    if (first == last_ptr)
                    {
                        if (next == nullptr)
                        {
                            return empty_val; // Queue is empty
                        }
                        // Tail is falling behind, help swing it forward
                        tail.compare_exchange_weak(
                            last_ptr, next,
                            std::memory_order_release,
                            std::memory_order_relaxed);
                    }
                    else
                    {
                        if (next == nullptr)
                        {
                            // Inconsistent state, retry
                            continue;
                        }

                        value_t val = next->value;
                        
                        // Try to swing header to the next node
                        if (header.compare_exchange_weak(
                                first, next,
                                std::memory_order_release,
                                std::memory_order_relaxed))
                        {
                            size.fetch_sub(1, std::memory_order_relaxed);
                            // Recycle the old dummy node
                            freelists[tid].push(first);
                            return val;
                        }
                    }
                }
            }
        }

        int get_size() override 
        { 
            return size.load(std::memory_order_relaxed); 
        }

        Node const *get_head() const 
        { 
            return header.load(std::memory_order_acquire); 
        }
        
        Node const *get_tail() const 
        { 
            return tail.load(std::memory_order_acquire); 
        }

        Queue(Queue const &other) = delete;
        Queue(Queue &&other) = delete;
        Queue &operator=(Queue const &other) = delete;
        Queue &operator=(Queue &&other) = delete;
    };
}