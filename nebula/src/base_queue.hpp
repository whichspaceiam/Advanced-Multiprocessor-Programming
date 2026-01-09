#pragma once
#include <limits>
#include <atomic>
#include <cassert>
#include <limits>
#include <iostream>
#include <omp.h>
#include <unordered_set>
#include <memory>
#include <vector>

namespace generics
{
    using value_t = int;
    constexpr value_t empty_val = -1000000; // if your queue only stores 0..1999

}; // namespace generics

class BaseQueue
{

public:
    using value_t = generics::value_t;

    
    virtual bool push(value_t v) = 0;
    virtual value_t pop() = 0;
    virtual int get_size() = 0;
    virtual ~BaseQueue() = default;
};