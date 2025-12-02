#pragma once
#include <limits>
#include <unordered_set>
#include <vector>
#include <mutex>
#include "sequential.hpp"

namespace global_lock
{
    using value_t = seq::value_t;

class ConcurrentQueue
{
    seq::Queue q;
    std::mutex m;

  public:
    ConcurrentQueue() = default;
    ConcurrentQueue(ConcurrentQueue const&) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;
    ConcurrentQueue(ConcurrentQueue &&) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue &&) = delete;

    void push(value_t v)
    {
        m.lock();
        q.push(v);
        m.unlock();
    }

    value_t pop()
    {
        value_t to_rtn;
        m.lock();
        to_rtn = q.pop();
        m.unlock();
        return to_rtn;
    }
};
}; // namespace global_lock