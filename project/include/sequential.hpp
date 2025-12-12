#pragma once

#include <vector>
#include "generics.hpp"
#include "base_queue.hpp"

namespace seq
{
    using value_t = generics::value_t;
    inline value_t empty_val = generics::empty_val;

    struct Node
    {
        Node *next;
        value_t value;
    };

    class FreeList
    {
        Node *header = nullptr;
        unsigned int size = 0;

    public:
        FreeList() = default;
        ~FreeList();
        FreeList(FreeList const &other) = delete;
        FreeList(FreeList &&other) = delete;
        FreeList &operator=(FreeList const &other) = delete;
        FreeList &operator=(FreeList &&other) = delete;

        void push(Node *n);
        Node *get(value_t val);
    };

    class Queue : public BaseQueue
    {
        Node *header;
        Node *tail;
        FreeList freelist;
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
        Node const *get_head() const;
        Node const *get_tail() const;

    };
}; // namespace seq