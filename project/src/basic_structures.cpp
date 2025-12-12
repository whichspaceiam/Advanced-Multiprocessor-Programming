#include "structures.hpp"

bs::FreeList::~FreeList()
{
    Node *walker = header;
    while (walker != nullptr)
    {
        Node *current = walker;
        walker = walker->next;
        delete current;
    }
}

bs::FreeList::FreeList(const FreeList &other)
{
    if (!other.header)
    {
        header = nullptr;
        return;
    }

    // Copy first node
    header = new Node;
    header->value = other.header->value;

    Node *cur_this = header;
    Node *cur_other = other.header->next;

    // Copy the rest of the nodes
    while (cur_other)
    {
        Node *n = new Node;
        n->value = cur_other->value;
        n->next = nullptr;

        cur_this->next = n;
        cur_this = n;
        cur_other = cur_other->next;
    }
}

// Copy assignment (deep copy)
bs::FreeList& bs::FreeList::operator=(FreeList const &other)

{
    if (this == &other)
        return *this;

    // Delete current list
    Node *walker = header;
    while (walker)
    {
        Node *current = walker;
        walker = walker->next;
        delete current;
    }

    // Copy new list (same as copy ctor)
    if (!other.header)
    {
        header = nullptr;
        return *this;
    }

    header = new Node;
    header->value = other.header->value;

    Node *cur_this = header;
    Node *cur_other = other.header->next;

    while (cur_other)
    {
        Node *n = new Node;
        n->value = cur_other->value;
        n->next = nullptr;

        cur_this->next = n;
        cur_this = n;
        cur_other = cur_other->next;
    }

    return *this;
}

// Move constructor
bs::FreeList::FreeList(FreeList &&other)
{
    header = other.header;
    other.header = nullptr;
}

// Move assignment
bs::FreeList& bs::FreeList::operator=(FreeList &&other)
{
    if (this != &other)
    {
        // delete current
        Node *walker = header;
        while (walker)
        {
            Node *current = walker;
            walker = walker->next;
            delete current;
        }

        header = other.header;
        other.header = nullptr;
    }
    return *this;
}
/// END OF COPY CONSTRUCTORS

void bs::FreeList::push(Node *n)
{
    n->next = header;
    header = n;
    size++;
}

bs::Node *bs::FreeList::get(value_t val)
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
