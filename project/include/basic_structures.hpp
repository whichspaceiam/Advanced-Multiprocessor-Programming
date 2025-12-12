#pragma once
#include "generics.hpp"

namespace bs
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
        FreeList(FreeList const &other);
        FreeList(FreeList &&other);
        FreeList &operator=(FreeList const &other);
        FreeList &operator=(FreeList &&other);

        void push(Node *n);
        Node *get(value_t val);
    };
};