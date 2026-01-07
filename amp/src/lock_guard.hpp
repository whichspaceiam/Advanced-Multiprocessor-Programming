#pragma once
#include <omp.h>
#include <limits>
#include <unordered_set>
#include <vector>
#include <mutex>
#include "sequential.hpp"
#include "generics.hpp"
#include "base_queue.hpp"


namespace global_lock
{
    using value_t = generics::value_t;

class Queue: public BaseQueue
{
    seq::Queue q;
    omp_lock_t global_lock;
    // std::mutex m;

  public:
    Queue(){
        omp_init_lock(&global_lock);
    };
    ~Queue(){
        omp_destroy_lock(&global_lock);
    }
    Queue(Queue const&) = delete;
    Queue& operator=(Queue const&) = delete;
    Queue(Queue &&) = delete;
    Queue& operator=(Queue &&) = delete;

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
    
    int get_size() override{
        int size;
        omp_set_lock(&global_lock);
        size = q.get_size();
        omp_unset_lock(&global_lock);
        return size;
    }
};
}; // namespace global_lock