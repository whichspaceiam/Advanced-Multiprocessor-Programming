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
template <typename T>
class TaggedPointer
{
private:
    std::atomic<uintptr_t> data;

    // Assume 48-bit pointers (x86-64), use upper 16 bits for version
    static constexpr uintptr_t PTR_MASK = 0x0000FFFFFFFFFFFF;      // Lower 48 bits
    static constexpr uintptr_t VERSION_MASK = 0xFFFF000000000000;  // Upper 16 bits
    static constexpr int VERSION_SHIFT = 48;

    static uintptr_t pack(T *ptr, uint16_t version)
    {
        uintptr_t ptrVal = reinterpret_cast<uintptr_t>(ptr);
        // Ensure pointer fits in 48 bits
        assert((ptrVal & PTR_MASK) == ptrVal && "Pointer exceeds 48-bit address space");
        return (ptrVal & PTR_MASK) | (static_cast<uintptr_t>(version) << VERSION_SHIFT);
    }

    static T *extractPointer(uintptr_t val)
    {
        return reinterpret_cast<T *>(val & PTR_MASK);
    }

    static uint16_t extractVersion(uintptr_t val)
    {
        return static_cast<uint16_t>((val & VERSION_MASK) >> VERSION_SHIFT);
    }

public:
    TaggedPointer(T *ptr = nullptr, uint16_t version = 0)
    {
        data.store(pack(ptr, version), std::memory_order_relaxed);
    }

    // Copy constructor
    TaggedPointer(const TaggedPointer &other)
    {
        data.store(other.data.load(std::memory_order_acquire), 
                   std::memory_order_release);  // ✅ Changed to release
    }

    // Copy assignment
    TaggedPointer &operator=(const TaggedPointer &other)
    {
        if (this != &other) {  // ✅ Added self-assignment check
            data.store(other.data.load(std::memory_order_acquire), 
                       std::memory_order_release);
        }
        return *this;
    }

    // Move constructor
    TaggedPointer(TaggedPointer &&other) noexcept
    {
        data.store(other.data.load(std::memory_order_acquire), 
                   std::memory_order_release);  // ✅ Changed to release
    }

    // Move assignment
    TaggedPointer &operator=(TaggedPointer &&other) noexcept
    {
        if (this != &other) {  // ✅ Added self-assignment check
            data.store(other.data.load(std::memory_order_acquire), 
                       std::memory_order_release);
        }
        return *this;
    }

    ~TaggedPointer() = default;

    T *getPointer(std::memory_order order = std::memory_order_acquire) const
    {
        return extractPointer(data.load(order));
    }

    uint16_t getVersion(std::memory_order order = std::memory_order_acquire) const
    {
        return extractVersion(data.load(order));
    }

    // ✅ More convenient: return pair
    std::pair<T*, uint16_t> load(std::memory_order order = std::memory_order_acquire) const
    {
        uintptr_t val = data.load(order);
        return {extractPointer(val), extractVersion(val)};
    }

    // ✅ Keep get() for output parameters (backward compatibility)
    void get(T **outPtr, uint16_t *outVersion, 
             std::memory_order order = std::memory_order_acquire) const
    {
        uintptr_t val = data.load(order);
        if (outPtr) *outPtr = extractPointer(val);
        if (outVersion) *outVersion = extractVersion(val);
    }

    // Store new tagged pointer
    void store(T *ptr, uint16_t version, std::memory_order order = std::memory_order_release)
    {
        data.store(pack(ptr, version), order);
    }

    bool compareAndSet(T *expectedPtr, uint16_t expectedVer,
                       T *newPtr, uint16_t newVer,
                       std::memory_order success = std::memory_order_acq_rel,
                       std::memory_order failure = std::memory_order_acquire)
    {
        uintptr_t expected = pack(expectedPtr, expectedVer);
        uintptr_t desired = pack(newPtr, newVer);
        return data.compare_exchange_strong(expected, desired, success, failure);
    }

    bool compareAndSetWeak(T *expectedPtr, uint16_t expectedVer,
                           T *newPtr, uint16_t newVer,
                           std::memory_order success = std::memory_order_acq_rel,
                           std::memory_order failure = std::memory_order_acquire)
    {
        uintptr_t expected = pack(expectedPtr, expectedVer);
        uintptr_t desired = pack(newPtr, newVer);
        return data.compare_exchange_weak(expected, desired, success, failure);
    }
};

namespace lock_free_aba
{
    using value_t = generics::value_t;
    value_t empty_val = generics::empty_val;

    // Align to cache line to avoid false sharing
    struct alignas(64) Node
    {
        TaggedPointer<Node> next;
        value_t value;

        Node() : next(nullptr, 0), value(empty_val) {}
    };

      class FreeList
    {
        TaggedPointer<Node> header;
        std::atomic<unsigned int> size;

    public:
        FreeList() : header(nullptr, 0), size(0) {}

        ~FreeList()
        {
            Node *walker = header.getPointer(std::memory_order_relaxed);
            while (walker)
            {
                Node *current = walker;
                walker = walker->next.getPointer(std::memory_order_relaxed);
                delete current;
            }
        }

        // Copy constructor (deep copy)
        FreeList(const FreeList &other) : header(nullptr, 0), size(0)
        {
            Node *otherWalker = other.header.getPointer(std::memory_order_acquire);
            if (!otherWalker)
                return;

            Node *newHead = new Node;
            newHead->value = otherWalker->value;
            newHead->next.store(nullptr, 0, std::memory_order_relaxed);

            Node *curThis = newHead;
            otherWalker = otherWalker->next.getPointer(std::memory_order_acquire);
            unsigned int count = 1;

            while (otherWalker)
            {
                Node *n = new Node;
                n->value = otherWalker->value;
                n->next.store(nullptr, 0, std::memory_order_relaxed);

                curThis->next.store(n, 0, std::memory_order_relaxed);
                curThis = n;
                otherWalker = otherWalker->next.getPointer(std::memory_order_acquire);
                count++;
            }

            header.store(newHead, 0, std::memory_order_release);
            size.store(count, std::memory_order_relaxed);
        }

        // Copy assignment
        FreeList &operator=(const FreeList &other)
        {
            if (this == &other)
                return *this;

            // Delete current list
            Node *walker = header.getPointer(std::memory_order_relaxed);
            while (walker)
            {
                Node *current = walker;
                walker = walker->next.getPointer(std::memory_order_relaxed);
                delete current;
            }

            Node *otherWalker = other.header.getPointer(std::memory_order_acquire);
            if (!otherWalker)
            {
                header.store(nullptr, 0, std::memory_order_release);
                size.store(0, std::memory_order_relaxed);
                return *this;
            }

            Node *newHead = new Node;
            newHead->value = otherWalker->value;
            newHead->next.store(nullptr, 0, std::memory_order_relaxed);

            Node *curThis = newHead;
            otherWalker = otherWalker->next.getPointer(std::memory_order_acquire);
            unsigned int count = 1;

            while (otherWalker)
            {
                Node *n = new Node;
                n->value = otherWalker->value;
                n->next.store(nullptr, 0, std::memory_order_relaxed);

                curThis->next.store(n, 0, std::memory_order_relaxed);
                curThis = n;
                otherWalker = otherWalker->next.getPointer(std::memory_order_acquire);
                count++;
            }

            header.store(newHead, 0, std::memory_order_release);
            size.store(count, std::memory_order_relaxed);

            return *this;
        }

        // Move constructor
        FreeList(FreeList &&other) noexcept
        {
            Node *ptr;
            uint16_t ver;
            other.header.get(&ptr, &ver, std::memory_order_acquire);
            header.store(ptr, ver, std::memory_order_relaxed);
            size.store(other.size.load(std::memory_order_relaxed), std::memory_order_relaxed);

            other.header.store(nullptr, 0, std::memory_order_release);
            other.size.store(0, std::memory_order_relaxed);
        }

        // Move assignment
        FreeList &operator=(FreeList &&other) noexcept
        {
            if (this != &other)
            {
                // Delete current
                Node *walker = header.getPointer(std::memory_order_relaxed);
                while (walker)
                {
                    Node *current = walker;
                    walker = walker->next.getPointer(std::memory_order_relaxed);
                    delete current;
                }

                Node *ptr;
                uint16_t ver;
                other.header.get(&ptr, &ver, std::memory_order_acquire);
                header.store(ptr, ver, std::memory_order_release);
                size.store(other.size.load(std::memory_order_relaxed), std::memory_order_relaxed);

                other.header.store(nullptr, 0, std::memory_order_release);
                other.size.store(0, std::memory_order_relaxed);
            }
            return *this;
        }

        void push(Node *n)
        {
            while (true)
            {
                Node *oldHead;
                uint16_t oldVer;
                header.get(&oldHead, &oldVer, std::memory_order_acquire);

                n->next.store(oldHead, 0, std::memory_order_relaxed);

                if (header.compareAndSet(oldHead, oldVer, n, oldVer + 1))
                {
                    size.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            }
        }

        Node *get(value_t val)
        {
            while (true)
            {
                Node *head;
                uint16_t headVer;
                header.get(&head, &headVer, std::memory_order_acquire);

                if (!head)
                    return nullptr;

                // Check if head matches
                if (head->value == val)
                {
                    Node *next;
                    uint16_t nextVer;
                    head->next.get(&next, &nextVer, std::memory_order_acquire);

                    if (header.compareAndSet(head, headVer, next, headVer + 1))
                    {
                        size.fetch_sub(1, std::memory_order_relaxed);
                        return head;
                    }
                    continue;
                }

                // Search through list (not lock-free for interior nodes)
                Node *walker = head;
                Node *prev = nullptr;

                while (walker)
                {
                    Node *next = walker->next.getPointer(std::memory_order_acquire);

                    if (walker->value == val && prev != nullptr)
                    {
                        // This part is tricky - we'd need a lock-free linked list deletion
                        // For simplicity, just return nullptr if not at head
                        // A full implementation would need DCAS or Harris's algorithm
                        return nullptr;
                    }

                    prev = walker;
                    walker = next;
                }

                return nullptr;
            }
        }

        unsigned int getSize() const
        {
            return size.load(std::memory_order_relaxed);
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
        h->next.store(nullptr, 0, std::memory_order_relaxed);
        h->value = empty_val;

        header.store(h, 0, std::memory_order_relaxed);
        tail.store(h, 0, std::memory_order_relaxed);

        size.store(0, std::memory_order_relaxed);

        int n_threads = omp_get_max_threads();
        freelists.resize(n_threads);
    }

    ~Queue()
    {
        Node *walker = header.getPointer(std::memory_order_relaxed);
        while (walker)
        {
            Node *current = walker;
            walker = walker->next.getPointer(std::memory_order_relaxed);
            delete current;
        }
    }

    bool push(value_t val) override
    {
        int tid = omp_get_thread_num();

        // Try to get cached node with this value
        Node *n = freelists[tid].get(val);
        if (n == nullptr)
        {
            // Not in cache, allocate new
            n = new Node;
            n->value = val;
        }
        // If found in cache, value is already set!
        
        n->next.store(nullptr, 0, std::memory_order_relaxed);

        while (true)
        {
            Node *last;
            uint16_t tailVer;
            tail.get(&last, &tailVer, std::memory_order_acquire);

            Node *next;
            uint16_t nextVer;
            last->next.get(&next, &nextVer, std::memory_order_acquire);

            // ✅ Removed redundant tail check
            // Version counter in CAS handles this

            if (next == nullptr)
            {
                // ✅ CAS from (next, nextVer) to (n, nextVer+1)
                if (last->next.compareAndSet(next, nextVer, n, nextVer + 1,
                                            std::memory_order_release,
                                            std::memory_order_acquire))
                {
                    // Successfully linked new node
                    tail.compareAndSet(last, tailVer, n, tailVer + 1,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
                    size.fetch_add(1, std::memory_order_relaxed);
                    return true;
                }
            }
            else
            {
                // Tail is lagging, help advance it
                tail.compareAndSet(last, tailVer, next, tailVer + 1,
                                  std::memory_order_release,
                                  std::memory_order_relaxed);
            }
        }
    }

    value_t pop() override
    {
        int tid = omp_get_thread_num();

        while (true)
        {
            Node *first;
            uint16_t headVer;
            header.get(&first, &headVer, std::memory_order_acquire);

            Node *last;
            uint16_t tailVer;
            tail.get(&last, &tailVer, std::memory_order_acquire);

            Node *next;
            uint16_t nextVer;
            first->next.get(&next, &nextVer, std::memory_order_acquire);

            // ✅ Check if head changed (ABA protection)
            Node *currentHead = header.getPointer(std::memory_order_acquire);
            if (first != currentHead)
                continue;

            if (first == last)
            {
                // Queue is empty or tail is lagging
                if (next == nullptr)
                {
                    // Queue is truly empty
                    return empty_val;
                }
                // Tail is lagging, help advance it
                tail.compareAndSet(last, tailVer, next, tailVer + 1,
                                  std::memory_order_release,
                                  std::memory_order_relaxed);
            }
            else
            {
                // Queue has items
                if (next == nullptr)
                {
                    // Inconsistent state, retry
                    continue;
                }

                // ✅ Read value before CAS (important!)
                value_t val = next->value;

                if (header.compareAndSet(first, headVer, next, headVer + 1,
                                        std::memory_order_release,
                                        std::memory_order_acquire))
                {
                    // Successfully dequeued
                    size.fetch_sub(1, std::memory_order_relaxed);
                    freelists[tid].push(first);  // ✅ Recycle dummy node
                    return val;
                }
            }
        }
    }

    int get_size() override
    {
        return size.load(std::memory_order_relaxed);
    }

    Queue(const Queue &) = delete;
    Queue(Queue &&) = delete;
    Queue &operator=(const Queue &) = delete;
    Queue &operator=(Queue &&) = delete;
};

} // namespace lock_free_aba