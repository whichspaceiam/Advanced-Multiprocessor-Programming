#include "global_lock.hpp"
#include "sequential.hpp"

gl::ConcurrentQueue::ConcurrentQueue()
{
    omp_init_lock(&global_lock);
};
gl::ConcurrentQueue::~ConcurrentQueue()
{
    omp_destroy_lock(&global_lock);
}

bool gl::ConcurrentQueue::push(value_t v)
{
    omp_set_lock(&global_lock);
    q.push(v);
    omp_unset_lock(&global_lock);
    return true;
}

gl::value_t gl::ConcurrentQueue::pop()
{
    value_t to_rtn;
    omp_set_lock(&global_lock);
    to_rtn = q.pop();
    omp_unset_lock(&global_lock);
    return to_rtn;
}

int gl::ConcurrentQueue::get_size() const
{
    return q.get_size();
}
