#pragma once

#include <atomic>
#include <omp.h>
#include <vector>

#include "structures.hpp"

namespace dl
{
    using value_t = generics::value_t;
    inline value_t empty_val = generics::empty_val;

    class Queue : public interface::BaseQueue
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
        Queue();

        ~Queue();

        bool push(value_t val) override;
        value_t pop() override;

        int get_size() const override;

        bs::Node const *get_head() const;
        bs::Node const *get_tail() const;

        Queue(Queue const &other) = delete;
        Queue(Queue const &&other) = delete;

        Queue &operator=(Queue const &other) = delete;
        Queue &operator=(Queue const &&other) = delete;
    };
}; // namespace finelock