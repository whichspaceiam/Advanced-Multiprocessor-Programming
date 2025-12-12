#pragma once

#include <vector>
#include "structures.hpp"

namespace seq
{
    class Queue : public interface::BaseQueue
    {
        bs::Node *header;
        bs::Node *tail;
        bs::FreeList freelist;
        unsigned int size;

    public:
        Queue();
        ~Queue();
        Queue(Queue const &other) = delete;
        Queue(Queue const &&other) = delete;
        Queue &operator=(Queue const &other) = delete;
        Queue &operator=(Queue const &&other) = delete;

        bool push(value_t val) override;
        value_t pop() override;
        int get_size() const override;
        bs::Node const *get_head() const;
        bs::Node const *get_tail() const;
    };
}; // namespace seq