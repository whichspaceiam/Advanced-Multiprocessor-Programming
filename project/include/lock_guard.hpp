#pragma once
#include <omp.h>
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
    omp_lock_t global_lock;
    // std::mutex m;

  public:
    ConcurrentQueue(){
        omp_init_lock(&global_lock);
    };
    ~ConcurrentQueue(){
        omp_destroy_lock(&global_lock);
    }
    ConcurrentQueue(ConcurrentQueue const&) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;
    ConcurrentQueue(ConcurrentQueue &&) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue &&) = delete;

    void push(value_t v)
    {
        omp_set_lock(&global_lock);
        q.push(v);
        omp_unset_lock(&global_lock);
    }
    
    value_t pop()
    {
        value_t to_rtn;
        omp_set_lock(&global_lock);
        to_rtn = q.pop();
        omp_unset_lock(&global_lock);
        return to_rtn;
    }
};
}; // namespace global_lock