// Part of ex 4

/*
The only logical difference with the sequential lock is in the push and pop
implementation. Apart from it of course we have to initialize and destroy the
locks.
*/

#pragma once
#include "base_queue.hpp"
#include "generics.hpp"
#include "basic_structures.hpp"
#include <atomic>
#include <cassert>
#include <limits>
#include <omp.h>
#include <unordered_set>
#include <vector>

namespace finelock
{
    // using value_t = generics::value_t;
    // inline value_t empty_val = generics::empty_val;

    // struct bs::Node
    // {
    //     bs::Node *next;
    //     value_t value;
    // };

    // class Bs::FreeList
    // {
    //     bs::Node *header = nullptr;
    //     unsigned int size = 0;

    // public:
    //     Bs::FreeList() = default;

    //     ~Bs::FreeList();

    //     Bs::FreeList(const Bs::FreeList &other);
       
    //     // Copy assignment (deep copy)
    //     Bs::FreeList &operator=(const Bs::FreeList &other);
        
    //     // Move constructor
    //     FreeList(FreeList &&other) noexcept
    //     {
    //         header = other.header;
    //         other.header = nullptr;
    //     }

    //     // Move assignment
    //     FreeList &operator=(FreeList &&other) noexcept
    //     {
    //         if (this != &other)
    //         {
    //             // delete current
    //             bs::Node *walker = header;
    //             while (walker)
    //             {
    //                 bs::Node *current = walker;
    //                 walker = walker->next;
    //                 delete current;
    //             }

    //             header = other.header;
    //             other.header = nullptr;
    //         }
    //         return *this;
    //     }
    //     /// END OF COPY CONSTRUCTORS

    //     void push(bs::Node *n)
    //     {
    //         n->next = header;
    //         header = n;
    //         size++;
    //     }

    //     bs::Node *get(value_t val)
    //     {
    //         if (header == nullptr)
    //             return nullptr;

    //         if (header->value == val)
    //         {
    //             bs::Node *to_rtn = header;
    //             header = header->next;
    //             size--;
    //             return to_rtn;
    //         }

    //         auto walker = header;
    //         while (walker->next != nullptr)
    //         {
    //             if (walker->next->value == val)
    //             {
    //                 auto to_rtn = walker->next;
    //                 walker->next = walker->next->next;
    //                 size--;
    //                 return to_rtn;
    //             }
    //             walker = walker->next;
    //         }

    //         return nullptr;
    //     }
    // };

    class Queue : public BaseQueue
    { // FIFO
        bs::Node *header;
        bs::Node *tail;

        std::vector<bs::FreeList> freelists;
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
            header = new bs::Node;
            header->next = nullptr;
            header->value = bs::empty_val;
            tail = header;
            size = 0;
            int n_threads = omp_get_max_threads();
            freelists.resize(n_threads);
        };

        ~Queue()
        {
            bs::Node *walker = header;
            while (walker != nullptr)
            {
                bs::Node *current = walker;
                walker = walker->next;
                delete current;
            }

            omp_destroy_lock(&header_lock);
            omp_destroy_lock(&tail_lock);
        };

        bool push(value_t val) override
        {
            int tid = omp_get_thread_num();
            bs::Node *n = freelists[tid].get(val);
            if (n == nullptr)
            {
                n = new bs::Node;
                n->value = val;
            }
            n->next = nullptr;

            omp_set_lock(&tail_lock);
            tail->next = n;
            tail = n;
            omp_unset_lock(&tail_lock);

            size++;

            return true;
        }

        value_t pop() override
        {
            int tid = omp_get_thread_num();

            omp_set_lock(&header_lock);

            bs::Node *current = header->next;
            if (current == nullptr)
            {
                omp_unset_lock(&header_lock);
                return bs::empty_val;
            }
            value_t val = current->value;

            if (size < 2)
            {

                omp_set_lock(&tail_lock);
                header->next = current->next;
                if (current == tail)
                    tail = header;

                omp_unset_lock(&tail_lock);
            }
            else
            {
                header->next = current->next;
            }

            omp_unset_lock(&header_lock);

            --size;
            freelists[tid].push(current);

            return val;
        }

        int get_size() const override { return size.load(); }

        bs::Node const *get_head() const { return header; }
        bs::Node const *get_tail() const { return tail; }

        Queue(Queue const &other) = delete;
        Queue(Queue const &&other) = delete;

        Queue &operator=(Queue const &other) = delete;
        Queue &operator=(Queue const &&other) = delete;
    };
}; // namespace finelock