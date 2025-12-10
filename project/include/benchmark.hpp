#pragma once
#include <array>
#include <atomic>
#include <filesystem>
#include <random>
#include <utility>
#include <iostream>
#include "sequential.hpp"
#include "lock_guard.hpp"

using value_t = int;

enum class SelectionStrategy
{
    PerThreadPushPop,
    ThreadSpecificAction
};

struct Config
{
    int num_threads;
    int repetitions;
    int max_time_in_s;
    int max_n_enqueues;
    int max_n_dequeues;
    int seed;
    SelectionStrategy selectionStrategy;
    // std::array<double, 3> mixOfOperations;
    // std::pair<int, int> keyRange;
    // KeyRangeType keyRangeType;
    // int numberOfPrefillItems;
};

struct AtomicCounters
{
    std::atomic<int> total_operations;
    std::atomic<int> succeeded_push;
    std::atomic<int> succeeded_pop;
    std::atomic<int> total_push;
    std::atomic<int> total_pop;

    std::atomic<value_t> push_sum;
    std::atomic<value_t> pop_sum;
};

struct Counter
{
    int total_operations{};
    int succeeded_push{};
    int succeeded_pop{};
    int total_push{};
    int total_pop{};
};

struct Results
{
    double avg_time{};
    int total_n_operations{};
    int avg_per_thread_operations{};

    int total_succeded_enqueues{};
    int total_succeded_dequeues{};

    int total_enqueues{};
    int total_dequeues{};

    double success_enqueue_rate{};
    double success_dequeue_rate{};
};

class Benchmark
{
  private:
    Config config;
    AtomicCounters counters;
    Results results;

    void update_atomic_global_counters(Counter const& thread_counter){
                counters.succeeded_pop.fetch_add(thread_counter.succeeded_pop, std::memory_order_relaxed);
                counters.succeeded_push.fetch_add(thread_counter.succeeded_push, std::memory_order_relaxed);
                counters.total_pop.fetch_add(thread_counter.total_pop, std::memory_order_relaxed);
                counters.total_push.fetch_add(thread_counter.total_push, std::memory_order_relaxed);
    }

    bool verify_correctness(AtomicCounters const & counters){
        if (counters.pop_sum!=counters.push_sum){
            return false;
        }
        return true;
    }
  public:
    Benchmark(Config &&config) : config(std::move(config)) {};
    void run()
    {
        std::mt19937 global_rng(config.seed);
        for (int i = 0; i < config.repetitions; i++)
        {
            reset_counters();
            #pragma omp parallel num_threads(config.num_threads)
            {
                int thread_id = omp_get_thread_num();
                std::mt19937 thread_rng(config.seed + thread_id + 1);
                Counter thread_counter{};


                double t_start = omp_get_wtime();
                while (omp_get_wtime() - t_start < config.max_time_in_s){
                // Do work push pop increase counters
                }

                double t_end = omp_get_wtime();
                update_atomic_global_counters(thread_counter);


                #pragma omp single
                {
                    results.avg_time += t_end-t_start;
                }

            }

            if(!verify_correctness(counters))
                std::cerr<<"Something went terribly wrong"<<std::endl;
            // parse_results();
        }

        // calculate_results();
    }
    void reset_counters();

    void print_results() const;
    void save_results(std::filesystem::path const &output) const;
};