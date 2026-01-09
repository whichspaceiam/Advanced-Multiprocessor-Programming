#pragma once
#include "base_queue.hpp"

namespace seq
{

    struct Node
    {
        Node *next;
        generics::value_t value;
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

        // Copy constructor (deep copy)
        FreeList(const FreeList &other)
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
        FreeList &operator=(const FreeList &other)
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
        FreeList(FreeList &&other) noexcept
        {
            header = other.header;
            other.header = nullptr;
        }

        // Move assignment
        FreeList &operator=(FreeList &&other) noexcept
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

        void push(Node *n)
        {
            n->next = header;
            header = n;
            size++;
        }

        Node *get(generics::value_t val)
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

        Node *pop()
        {
            if (header == nullptr)
                return nullptr;

            Node *current = header;
            header = header->next; // Move the header to the next node
            size--;
            return current; // Return the node that was popped
        }
    };

    class Queue : public BaseQueue
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
            header->value = generics::empty_val;
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
            Node *n = freelist.pop();
            if (n == nullptr)
            {
                n = new Node;
            }
            n->value = val;
            n->next = nullptr;
            tail->next = n;
            tail = n;
            size++;

            return true;
        }
        value_t pop() override
        {
            if (header->next == nullptr)
                return generics::empty_val;

            Node *current = header->next;
            value_t val = current->value;

            header->next = current->next;
            if (current == tail)
                tail = header;

            freelist.push(current);
            size--;
            return val;
        }

        int get_size() override { return size; }

        Node const *get_head() const { return header; }
        Node const *get_tail() const { return tail; }

        Queue(Queue const &other) = delete;
        Queue(Queue &&other) = delete;

        Queue &operator=(Queue const &other) = delete;
        Queue &operator=(Queue &&other) = delete;
    };
}; // namespace seq