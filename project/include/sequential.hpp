#pragma once
#include <limits>
#include <unordered_set>
#include <vector>
#include "generics.hpp"
#include "base_queue.hpp"

namespace seq
{
using value_t = generics::value_t; // To be changed if one will
value_t empty_val = generics::empty_val;



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

    ~FreeList()
    {
        Node *walker = header;
        while (walker != nullptr)
        {
            Node *current = walker;
            walker = walker->next;
            delete current;
        }
    }

    FreeList(FreeList const &other) = delete;
    FreeList(FreeList &&other) = delete;
    FreeList &operator=(FreeList const &other) = delete;
    FreeList &operator=(FreeList &&other) = delete;

    void push(Node *n)
    {
        n->next = header;
        header = n;
        size++;
    }

    Node *get(value_t val)
    {
        if (header == nullptr)
            return nullptr;

        if (header->value == val)
        {
            Node *to_rtn = header;
            header = header->next;
            size--;
            return to_rtn;
        }

        auto walker = header;
        while (walker->next != nullptr)
        {
            if (walker->next->value == val)
            {
                auto to_rtn = walker->next;
                walker->next = walker->next->next;
                size--;
                return to_rtn;
            }
            walker = walker->next;
        }

        return nullptr;
    }
};

class Queue: public BaseQueue
{ // FIFO
    Node *header;
    Node *tail;
    FreeList freelist;
    unsigned int size;

  public:
    Queue()
    {
        header = new Node;
        header->next = nullptr;
        header->value = empty_val;
        tail = header;
        size = 0;
    };

    ~Queue()
    {
        Node *walker = header;
        while (walker != nullptr)
        {
            Node *current = walker;
            walker = walker->next;
            delete current;
        }
    };

    bool push(value_t val) override
    {
        Node *n = freelist.get(val);
        if (n == nullptr)
        {
            n = new Node;
            n->value = val;
        }
        n->next = nullptr;
        tail->next = n;
        tail = n;
        size++;

        return true;
    }
    value_t pop() override
    {
        if (header->next == nullptr)
            return empty_val;

        Node *current = header->next;
        value_t val = current->value;

        header->next = current->next;
        if (current == tail)
            tail = header;

        freelist.push(current);
        size--;
        return val;
    }

    int get_size() const override {return size;}

    Node const *get_head() const { return header; }
    Node const *get_tail() const { return tail; }

    Queue(Queue const &other) = delete;
    Queue(Queue const &&other) = delete;

    Queue &operator=(Queue const &other) = delete;
    Queue &operator=(Queue const &&other) = delete;
};
}; // namespace seq