#pragma once
#include <immintrin.h>
#include "base_queue.hpp"
#include "tagged_ptr.hpp"

namespace lock_free_aba
{

    using value_t = generics::value_t;
    value_t empty_val = generics::empty_val;

    struct alignas(64) Node
    {
        TaggedPointer<Node> next;
        value_t value;
        Node() : next(nullptr, 0), value(empty_val) {}
    };

    class alignas(64) FreeList
    {
        TaggedPointer<Node> header;
        size_t size{};

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

        FreeList(const FreeList &) = delete;
        FreeList &operator=(const FreeList &) = delete;
        FreeList(FreeList &&other) noexcept
            : header(nullptr, 0), size(0)
        {
            auto [otherHead, otherVer] = other.header.load(std::memory_order_relaxed);
            header.store(otherHead, otherVer, std::memory_order_relaxed);
            size = other.size;
            other.header.store(nullptr, 0, std::memory_order_relaxed);
            other.size = 0;
        }

        // ✅ FIXED: Just push any node
        void push(Node *n)
        {
            if (!n)
                return;

            Node *oldHead = header.getPointer(std::memory_order_relaxed);
            n->next.store(oldHead, 0, std::memory_order_relaxed);
            header.store(n, 0, std::memory_order_relaxed);
            size++;
        }

        // ✅ FIXED: Just pop any node (no search!)
        Node *pop()
        {
            Node *head = header.getPointer(std::memory_order_relaxed);
            if (!head)
                return nullptr;

            Node *next = head->next.getPointer(std::memory_order_relaxed);
            header.store(next, 0, std::memory_order_relaxed);
            size--;

            // Caller will set the value
            return head;
        }

        size_t get_size() const { return size; }
    };
    class Queue : public BaseQueue
    {
        alignas(64) TaggedPointer<Node> header;
        alignas(64) TaggedPointer<Node> tail;
        alignas(64) std::vector<FreeList> freelists;
        alignas(64) std::atomic<int> size;

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
            freelists.reserve(n_threads);
            for (int i = 0; i < n_threads; ++i)
            {
                freelists.emplace_back();
            }
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

            for (auto &freelist : freelists)
            {
                Node *node;
                while ((node = freelist.pop()) != nullptr)
                {
                    delete node;
                }
            }
        }

        bool push(value_t val) override
        {
            int tid = omp_get_thread_num();
            FreeList &freelist = freelists[tid];

            Node *n = freelist.pop();
            if (n == nullptr)
            {
                n = new Node;
            }

            n->value = val;
            n->next.store(nullptr, 0, std::memory_order_relaxed);

            int backoff = 1;

            while (true)
            {
                Node *last;
                uint16_t tailVer;
                tail.get(&last, &tailVer, std::memory_order_acquire);

                Node *next;
                uint16_t nextVer;
                last->next.get(&next, &nextVer, std::memory_order_acquire);

                Node *currentTail;
                uint16_t currentTailVer;
                tail.get(&currentTail, &currentTailVer, std::memory_order_relaxed);

                if (currentTail != last)
                {
                    backoff = 1;
                    continue;
                }

                if (next == nullptr)
                {
                    if (last->next.compareAndSet(next, nextVer, n, nextVer + 1,
                                                 std::memory_order_release,
                                                 std::memory_order_acquire))
                    {
                        tail.compareAndSet(last, currentTailVer, n, currentTailVer + 1,
                                           std::memory_order_release,
                                           std::memory_order_relaxed);

                        size.fetch_add(1, std::memory_order_relaxed);
                        return true;
                    }

                    for (int i = 0; i < backoff; i++)
                    {
                        _mm_pause();
                    }
                    backoff = std::min(backoff * 2, 64);
                }
                else
                {
                    tail.compareAndSet(last, tailVer, next, tailVer + 1,
                                       std::memory_order_release,
                                       std::memory_order_relaxed);
                    backoff = 1;
                }
            }
        }

        value_t pop() override
        {
            int tid = omp_get_thread_num();
            FreeList &freelist = freelists[tid];
            int backoff = 1;

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

                Node *currentHead;
                uint16_t currentHeadVer;
                header.get(&currentHead, &currentHeadVer, std::memory_order_relaxed);

                if (first != currentHead)
                {
                    continue;
                }

                if (first == last)
                {
                    if (next == nullptr)
                    {
                        return empty_val;
                    }

                    tail.compareAndSet(last, tailVer, next, tailVer + 1,
                                       std::memory_order_release,
                                       std::memory_order_relaxed);
                }
                else
                {
                    if (next == nullptr)
                    {
                        continue;
                    }

                    value_t val = next->value;

                    if (header.compareAndSet(first, headVer, next, headVer + 1,
                                             std::memory_order_release,
                                             std::memory_order_acquire))
                    {
                        freelist.push(first);
                        size.fetch_sub(1, std::memory_order_relaxed);
                        return val;
                    }
                }

                for (int i = 0; i < backoff; i++)
                {
                    _mm_pause();
                }
                backoff = std::min(backoff * 2, 64);
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

}
