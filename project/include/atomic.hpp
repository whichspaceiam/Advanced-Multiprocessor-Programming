/* Ex 5 Lock free implementation*/

// Part of ex 4

/*
The only logical difference with the sequential lock is in the push and pop
implementation. Apart from it of course we have to initialize and destroy the
locks.
*/

#pragma once
#include "structures.hpp"
#include <atomic>
#include <cassert>
#include <omp.h>
#include <unordered_set>
#include <vector>

namespace lock_free
{
    using namespace as;
    using value_t = generics::value_t; // To be changed if one will
    inline value_t empty_val = generics::empty_val;

    class Queue : public interface::BaseQueue
    {
        as::TaggedPointer<as::Node> header;
        as::TaggedPointer<as::Node> tail;

        std::vector<as::FreeList> freelists;
        std::atomic<int> size;

    public:
        Queue();
        ~Queue();
        bool push(value_t val) override;
        value_t pop() override;
        int get_size() const override;
    };

}; // namespace finelock