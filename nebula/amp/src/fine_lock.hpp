#pragma once
#include "base_queue.hpp"

namespace fine_lock
{

    class Queue : public BaseQueue
    {
        seq::Node *header;
        seq::Node *tail;
        std::vector<seq::FreeList> freelists;
        std::atomic<int> size;
        omp_lock_t header_lock;
        omp_lock_t tail_lock;

        seq::Node *_get_node(value_t val, int tid)
        {
            seq::Node *n = freelists[tid].pop();
            if (n == nullptr)
            {
                n = new seq::Node;
            }
            n->value = val;
            return n;
        }

    public:
        Queue()
        {
            omp_init_lock(&header_lock);
            omp_init_lock(&tail_lock);
            header = new seq::Node;
            header->next = nullptr; // Regular pointer assignment
            header->value = generics::empty_val;
            tail = header;
            size.store(0);

            int n_threads = omp_get_max_threads();
            freelists.resize(n_threads);
        }

        ~Queue()
        {
            seq::Node *walker = header;
            while (walker != nullptr)
            {
                seq::Node *current = walker;
                walker = walker->next; // Regular pointer access
                delete current;
            }

            omp_destroy_lock(&header_lock);
            omp_destroy_lock(&tail_lock);
        }

        bool push(value_t val) override
        {
            int tid = omp_get_thread_num();
            seq::Node *n = _get_node(val, tid);
            n->next = nullptr;

            omp_set_lock(&tail_lock);
            tail->next = n;
            tail = n;
            omp_unset_lock(&tail_lock);
            size.fetch_add(1);

            return true;
        }

        value_t pop() override
        {
            int tid = omp_get_thread_num();

            omp_set_lock(&header_lock);

            seq::Node *current = header->next;
            if (current == nullptr)
            {
                omp_unset_lock(&header_lock);
                return generics::empty_val;
            }

            value_t val = current->value;
            seq::Node *next = current->next;

            // If removing potentially last element, need both locks
            if (next == nullptr)
            {
                omp_set_lock(&tail_lock);

                // Recheck under both locks (state may have changed)
                next = current->next;
                if (next == nullptr)
                {
                    // Actually the last element
                    header->next = nullptr;
                    tail = header;
                }
                else
                {
                    // Another thread pushed, just remove normally
                    header->next = next;
                }

                size.fetch_sub(1);
                omp_unset_lock(&tail_lock);
            }
            else
            {
                // Not last element, safe to remove
                header->next = next;
                size.fetch_sub(1);
            }

            omp_unset_lock(&header_lock);

            freelists[tid].push(current);
            return val;
        }

        int get_size() override
        {
            return size.load();
        }

        seq::Node const *get_head() const { return header; }
        seq::Node const *get_tail() const { return tail; }

        Queue(Queue const &other) = delete;
        Queue(Queue &&other) = delete;
        Queue &operator=(Queue const &other) = delete;
        Queue &operator=(Queue &&other) = delete;
    };

}; // namespace finelock
