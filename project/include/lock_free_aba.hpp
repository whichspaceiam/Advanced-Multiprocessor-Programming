/* Ex 5 Lock free implementation*/

// Part of ex 4

/*
The only logical difference with the sequential lock is in the push and pop
implementation. Apart from it of course we have to initialize and destroy the
locks.
*/

#pragma once
#include "base_queue.hpp"
#include "generics.hpp"
#include <atomic>
#include <cassert>
#include <limits>
#include <omp.h>
#include <unordered_set>
#include <vector>

// #include <atomic>
// #include <cstdint>

template <typename T>
class TaggedPointer
{
private:
    std::atomic<uintptr_t> data;

    static constexpr uintptr_t MARK_MASK = 0x1;   // lowest bit
    static constexpr uintptr_t VERSION_BITS = 16; // example
    static constexpr uintptr_t VERSION_MASK = ((1ULL << VERSION_BITS) - 1) << 1;
    static constexpr uintptr_t PTR_MASK = ~((1ULL << (VERSION_BITS + 1)) - 1);

    static uintptr_t pack(T *ptr, uint16_t version, bool mark)
    {
        return (reinterpret_cast<uintptr_t>(ptr) & PTR_MASK) |
               ((static_cast<uintptr_t>(version) << 1) & VERSION_MASK) |
               (mark ? 1 : 0);
    }

    static T *extractPointer(uintptr_t val)
    {
        return reinterpret_cast<T *>(val & PTR_MASK);
    }

    static uint16_t extractVersion(uintptr_t val)
    {
        return (val & VERSION_MASK) >> 1;
    }

    static bool extractMark(uintptr_t val)
    {
        return (val & MARK_MASK) != 0;
    }

public:
    TaggedPointer(T *ptr = nullptr, uint16_t version = 0, bool mark = false)
    {
        data.store(pack(ptr, version, mark), std::memory_order_relaxed);
    }

    TaggedPointer(TaggedPointer &&other) noexcept
    {
        data.store(other.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    TaggedPointer &operator=(TaggedPointer &&other) noexcept
    {
        data.store(other.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    // Never delete pointer in destructor!
    ~TaggedPointer() = default;
    T *getPointer() const
    {
        return extractPointer(data.load(std::memory_order_relaxed));
    }

    void get(T **outPtr, uint16_t *outVersion, bool *outMark) const
    {
        uintptr_t val = data.load(std::memory_order_relaxed);
        *outPtr = extractPointer(val);
        *outVersion = extractVersion(val);
        *outMark = extractMark(val);
    }

    bool compareAndSet(T *expectedPtr, uint16_t expectedVer, bool expectedMark,
                       T *newPtr, uint16_t newVer, bool newMark)
    {
        uintptr_t expected = pack(expectedPtr, expectedVer, expectedMark);
        uintptr_t desired = pack(newPtr, newVer, newMark);
        return data.compare_exchange_strong(expected, desired,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire);
    }
};

namespace lock_free_aba
{
    using value_t = generics::value_t; // To be changed if one will
    value_t empty_val = generics::empty_val;

    // struct Node
    // {
    //     std::atomic<Node *> next;
    //     value_t value;
    // };

    struct alignas(1 << 17) Node
    {
        TaggedPointer<Node> next; // pointer + version + mark
        int value;
    };

    class FreeList
    {
        TaggedPointer<Node> header{};
        unsigned int size = 0;

    public:
        FreeList() = default;

        ~FreeList()
        {
            Node *walker = header.getPointer();
            while (walker)
            {
                Node *current = walker;
                walker = walker->next.getPointer();
                delete current;
            }
        }

        // Copy constructor (deep copy)
        FreeList(const FreeList &other)
        {
            Node *otherWalker = other.header.getPointer();
            if (!otherWalker)
            {
                header = TaggedPointer<Node>(nullptr);
                return;
            }

            // Copy first node
            Node *newHead = new Node;
            newHead->value = otherWalker->value;
            header = TaggedPointer<Node>(newHead);

            Node *curThis = newHead;
            otherWalker = otherWalker->next.getPointer();

            while (otherWalker)
            {
                Node *n = new Node;
                n->value = otherWalker->value;
                n->next = TaggedPointer<Node>(nullptr);

                curThis->next = TaggedPointer<Node>(n);
                curThis = n;
                otherWalker = otherWalker->next.getPointer();
            }
        }

        // Copy assignment (deep copy)
        FreeList &operator=(const FreeList &other)
        {
            if (this == &other)
                return *this;

            // Delete current list
            Node *walker = header.getPointer();
            while (walker)
            {
                Node *current = walker;
                walker = walker->next.getPointer();
                delete current;
            }

            Node *otherWalker = other.header.getPointer();
            if (!otherWalker)
            {
                header = TaggedPointer<Node>(nullptr);
                return *this;
            }

            Node *newHead = new Node;
            newHead->value = otherWalker->value;
            header = TaggedPointer<Node>(newHead);

            Node *curThis = newHead;
            otherWalker = otherWalker->next.getPointer();

            while (otherWalker)
            {
                Node *n = new Node;
                n->value = otherWalker->value;
                n->next = TaggedPointer<Node>(nullptr);

                curThis->next = TaggedPointer<Node>(n);
                curThis = n;
                otherWalker = otherWalker->next.getPointer();
            }

            return *this;
        }

        // Move constructor
        FreeList(FreeList &&other) noexcept
        {
            header = std::move(other.header);
            other.header = TaggedPointer<Node>(nullptr);
        }

        // Move assignment
        FreeList &operator=(FreeList &&other) noexcept
        {
            if (this != &other)
            {
                // Delete current
                Node *walker = header.getPointer();
                while (walker)
                {
                    Node *current = walker;
                    walker = walker->next.getPointer();
                    delete current;
                }

                header = std::move(other.header);
                other.header = TaggedPointer<Node>(nullptr);
            }
            return *this;
        }

        void push(Node *n)
        {
            n->next = std::move(header);
            header = TaggedPointer<Node>(n);
            size++;
        }

        Node *get(value_t val)
        {
            Node *walker = header.getPointer();
            if (!walker)
                return nullptr;

            // Check head first
            if (walker->value == val)
            {
                Node *toRtn = walker;
                header = std::move(walker->next);
                size--;
                return toRtn;
            }

            while (walker->next.getPointer())
            {
                Node *nextPtr = walker->next.getPointer();
                if (nextPtr->value == val)
                {
                    walker->next = std::move(nextPtr->next);
                    size--;
                    return nextPtr;
                }
                walker = nextPtr;
            }

            return nullptr;
        }
    };
    class Queue : public BaseQueue
    {
        TaggedPointer<Node> header;
        TaggedPointer<Node> tail;

        std::vector<FreeList> freelists;
        std::atomic<int> size;

    public:
        Queue()
        {
            Node *h = new Node;
            h->next = TaggedPointer<Node>(nullptr, 0);
            h->value = empty_val;

            header = TaggedPointer<Node>(h, 0);
            tail = TaggedPointer<Node>(h, 0);

            size.store(0, std::memory_order_relaxed);

            int n_threads = omp_get_max_threads();
            freelists.resize(n_threads);
        }

        ~Queue()
        {
            Node *walker = header.getPointer();
            while (walker)
            {
                Node *current = walker;
                walker = walker->next.getPointer();
                delete current;
            }
        }

        bool push(value_t val) override
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

        value_t pop() override
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

        int get_size() const override
        {
            return size.load(std::memory_order_relaxed);
        }
    };

}; // namespace finelock