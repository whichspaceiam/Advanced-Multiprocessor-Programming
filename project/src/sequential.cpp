#include "sequential.hpp"

seq::FreeList::~FreeList()
{
    Node *walker = header;
    while (walker != nullptr)
    {
        Node *current = walker;
        walker = walker->next;
        delete current;
    }
}

void seq::FreeList::push(Node *n)
{
    n->next = header;
    header = n;
    size++;
}

seq::Node *seq::FreeList::get(value_t val)
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

seq::Queue::Queue()
{
    header = new Node;
    header->next = nullptr;
    header->value = empty_val;
    tail = header;
    size = 0;
};

seq::Queue::~Queue()
{
    Node *walker = header;
    while (walker != nullptr)
    {
        Node *current = walker;
        walker = walker->next;
        delete current;
    }
};

bool seq::Queue::push(value_t val)
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

seq::value_t seq::Queue::pop()
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

int seq::Queue::get_size() const { return size; }

seq::Node const *seq::Queue::get_head() const { return header; }
seq::Node const *seq::Queue::get_tail() const { return tail; }
