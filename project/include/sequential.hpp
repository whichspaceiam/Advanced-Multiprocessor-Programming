#pragma once
#include <limits>
#include <unordered_set>
#include <vector>

namespace seq
{
using value_t = double; // To be changed
value_t empty_val = std::numeric_limits<value_t>::infinity();

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

class Queue
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

    void push(value_t val)
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
    }
    value_t pop()
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

    Node const *get_head() const { return header; }
    Node const *get_tail() const { return tail; }

    Queue(Queue const &other) = delete;
    Queue(Queue const &&other) = delete;

    Queue &operator=(Queue const &other) = delete;
    Queue &operator=(Queue const &&other) = delete;
};
}; // namespace seq