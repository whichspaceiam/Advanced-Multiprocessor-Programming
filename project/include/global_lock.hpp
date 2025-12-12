#pragma once
#include <omp.h>
#include "sequential.hpp"
#include "generics.hpp"
#include "base_queue.hpp"


namespace global_lock
{
    using value_t = generics::value_t;

class ConcurrentQueue: public BaseQueue
{
    seq::Queue q;
    omp_lock_t global_lock;

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

    bool push(value_t v) override
    {
        omp_set_lock(&global_lock);
        q.push(v);
        omp_unset_lock(&global_lock);
        return true;
    }
    
    value_t pop() override
    {
        value_t to_rtn;
        omp_set_lock(&global_lock);
        to_rtn = q.pop();
        omp_unset_lock(&global_lock);
        return to_rtn;
    }

    int get_size() const override{
        return q.get_size();
    }
};
}; // namespace global_lock