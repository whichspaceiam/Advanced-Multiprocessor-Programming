#include "fine_lock.hpp"
//#include "04_ConcurrentQueue.hpp"
#include <iostream>
#include <omp.h>
#include <vector>
#include <numeric>

int main() {
    fine_lock::Queue q;

    int num_threads = 20;
    int pushes_per_thread = 1000;

    // Vector to store all values each thread will push
    std::vector<std::vector<int>> thread_values(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        thread_values[t].resize(pushes_per_thread);
        for (int i = 0; i < pushes_per_thread; ++i) {
            thread_values[t][i] = t * pushes_per_thread + i;
        }
    }

    // Parallel push
#pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        for (int v : thread_values[tid]) {
            q.push(v);
        }
    }

    // Collect popped values
    std::vector<int> popped;
    int total_pushed = num_threads * pushes_per_thread;

    while (popped.size() < total_pushed) {
        int val = q.pop();
        if (val != fine_lock::empty_val) {
            popped.push_back(val);
        }
    }

    std::cout << "All elements popped: " << popped.size() << std::endl;

    // Optional: Sum check
    long long sum_pushed = 0;
    long long sum_popped = 0;

    for (int t = 0; t < num_threads; ++t)
        sum_pushed += std::accumulate(thread_values[t].begin(), thread_values[t].end(), 0LL);

    for (int v : popped)
        sum_popped += v;

    std::cout << "Sum pushed: " << sum_pushed << ", sum popped: " << sum_popped << std::endl;

    if (sum_pushed == sum_popped)
        std::cout << "Queue OK!" << std::endl;
    else
        std::cout << "Queue CORRUPTED!" << std::endl;

    return 0;
}
