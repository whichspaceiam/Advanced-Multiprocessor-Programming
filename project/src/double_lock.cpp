#include "double_lock.hpp"

dl::Queue::Queue()
{
    omp_init_lock(&header_lock);
    omp_init_lock(&tail_lock);
    header = new bs::Node;
    header->next = nullptr;
    header->value = bs::empty_val;
    tail = header;
    size = 0;
    int n_threads = omp_get_max_threads();
    freelists.resize(n_threads);
};

dl::Queue::~Queue()
{
    bs::Node *walker = header;
    while (walker != nullptr)
    {
        bs::Node *current = walker;
        walker = walker->next;
        delete current;
    }

    omp_destroy_lock(&header_lock);
    omp_destroy_lock(&tail_lock);
};

bool dl::Queue::push(value_t val)
{
    int tid = omp_get_thread_num();
    bs::Node *n = freelists[tid].get(val);
    if (n == nullptr)
    {
        n = new bs::Node;
        n->value = val;
    }
    n->next = nullptr;

    omp_set_lock(&tail_lock);
    tail->next = n;
    tail = n;
    omp_unset_lock(&tail_lock);

    size++;

    return true;
}

dl::value_t dl::Queue::pop()
{
    int tid = omp_get_thread_num();

    omp_set_lock(&header_lock);

    bs::Node *current = header->next;
    if (current == nullptr)
    {
        omp_unset_lock(&header_lock);
        return bs::empty_val;
    }
    value_t val = current->value;

    if (size < 2)
    {

        omp_set_lock(&tail_lock);
        header->next = current->next;
        if (current == tail)
            tail = header;

        omp_unset_lock(&tail_lock);
    }
    else
    {
        header->next = current->next;
    }

    omp_unset_lock(&header_lock);

    --size;
    freelists[tid].push(current);

    return val;
}

int dl::Queue::get_size() const { return size.load(); }

bs::Node const *dl::Queue::get_head() const { return header; }
bs::Node const *dl::Queue::get_tail() const { return tail; }