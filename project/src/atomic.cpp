#include "atomic.hpp"

lock_free::Queue::Queue()
{
    as::Node *h = new as::Node;
    h->next = as::TaggedPointer<as::Node>(nullptr, 0);
    h->value = empty_val;

    header = as::TaggedPointer<as::Node>(h, 0);
    tail = as::TaggedPointer<as::Node>(h, 0);

    size.store(0, std::memory_order_relaxed);

    int n_threads = omp_get_max_threads();
    freelists.resize(n_threads);
}

lock_free::Queue::~Queue()
{
    Node *walker = header.getPointer();
    while (walker)
    {
        Node *current = walker;
        walker = walker->next.getPointer();
        delete current;
    }
}

bool lock_free::Queue::push(value_t val)
{
    Node *n = new Node;
    n->value = val;
    n->next = TaggedPointer<Node>(nullptr, 0);

    while (true)
    {
        Node *last;
        uint16_t tailVer;
        bool tailMark;
        tail.get(&last, &tailVer, &tailMark);

        Node *next;
        uint16_t nextVer;
        bool nextMark;
        last->next.get(&next, &nextVer, &nextMark);

        if (!next)
        {
            // Try to append node
            if (last->next.compareAndSet(nullptr, nextVer, nextMark, n, nextVer + 1, false))
            {
                // CAS success, try to advance tail (optional)
                tail.compareAndSet(last, tailVer, tailMark, n, tailVer + 1, false);
                size.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }
        else
        {
            // Tail is behind, advance it
            tail.compareAndSet(last, tailVer, tailMark, next, tailVer + 1, false);
        }
    }
}

lock_free::value_t lock_free::Queue::pop()
{
    while (true)
    {
        Node *first;
        uint16_t firstVer;
        bool firstMark;
        header.get(&first, &firstVer, &firstMark);

        Node *last;
        uint16_t tailVer;
        bool tailMark;
        tail.get(&last, &tailVer, &tailMark);

        Node *next;
        uint16_t nextVer;
        bool nextMark;
        first->next.get(&next, &nextVer, &nextMark);

        if (!next)
            return empty_val; // queue empty

        if (first == last)
        {
            // Tail behind, advance it
            tail.compareAndSet(last, tailVer, tailMark, next, tailVer + 1, false);
        }
        else
        {
            value_t val = next->value;
            if (header.compareAndSet(first, firstVer, firstMark, next, firstVer + 1, false))
            {
                size.fetch_sub(1, std::memory_order_relaxed);
                return val;
            }
        }
    }
}

int lock_free::Queue::get_size() const
{
    return size.load(std::memory_order_relaxed);
}
