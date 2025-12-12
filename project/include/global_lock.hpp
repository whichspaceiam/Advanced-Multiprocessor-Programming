#pragma once
#include <omp.h>
#include "sequential.hpp"
#include "generics.hpp"
#include "base_queue.hpp"

namespace gl
{
    using value_t = generics::value_t;

    class ConcurrentQueue : public BaseQueue
    {
        seq::Queue q;
        omp_lock_t global_lock;

    public:
        ConcurrentQueue();
        ~ConcurrentQueue();
        ConcurrentQueue(ConcurrentQueue const &) = delete;
        ConcurrentQueue &operator=(ConcurrentQueue const &) = delete;
        ConcurrentQueue(ConcurrentQueue &&) = delete;
        ConcurrentQueue &operator=(ConcurrentQueue &&) = delete;

        bool push(value_t v) override;

        value_t pop() override;

        int get_size() const override;
    };
}; // namespace global_lock