#pragma once
#include "lock_guard.hpp"
#include "sequential.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <random>
#include <utility>

using value_t = int;

enum class ConfigRecipe
{
    Balanced,
    ThreadSpecific
};

struct Config
{
    int num_threads;
    int repetitions;
    int max_time_in_s;
    int seed;
    std::vector<int> batch_enque; // per thread batch
    std::vector<int> batch_deque; // per thread batch

    bool is_config_correct()
    {
        if (batch_deque.size() != num_threads)
            return false;
        if (batch_enque.size() != num_threads)
            return false;

        int sum_pushed_batches =
            std::accumulate(batch_enque.begin(), batch_enque.end(), 0);
        int sum_poped_batches =
            std::accumulate(batch_deque.begin(), batch_deque.end(), 0);

        if (sum_poped_batches != sum_pushed_batches)
        {
            std::cout << " Not balanced batches are declared" << std::endl;
            return false;
        }

        if (max_time_in_s < 0 || max_time_in_s > 100)
        {
            std::cout << "Time specification is not reasonable" << std::endl;
            return false;
        }

        if (repetitions < 0 || repetitions > 100)
        {
            std::cout << "Repetition specification seems irresonable"
                      << std::endl;
            return false;
        }

        if (num_threads < 0)
        {
            std::cout << "Why negative number of threads (?)" << std::endl;
            return false;
        }

        return true;
    }
};

class ConfigFactory
{
  private:
    int num_threads;
    int repetitions;
    int max_time_in_s;
    int seed;
    ConfigRecipe config_recipe;

  public:
    ConfigFactory(int num_threads, int repetitions, int max_time_in_s, int seed,
                  ConfigRecipe config_recipe)
        : num_threads(num_threads), repetitions(repetitions),
          max_time_in_s(max_time_in_s), seed(seed), config_recipe(config_recipe)
    {
    }

    Config operator()()
    {
        Config to_rtn{};
        to_rtn.num_threads = num_threads;
        to_rtn.repetitions = repetitions;
        to_rtn.max_time_in_s = max_time_in_s;
        to_rtn.seed = seed;

        switch (config_recipe)
        {
        case ConfigRecipe::Balanced:
        {
            to_rtn.batch_enque.resize(num_threads, 128);
            to_rtn.batch_deque.resize(num_threads, 128);
            break;
        }

        case ConfigRecipe::ThreadSpecific:
        {
            to_rtn.batch_enque.resize(num_threads, 0);
            to_rtn.batch_deque.resize(num_threads, 0);

            int half_threads = num_threads / 2;

            for (int i = 0; i < half_threads; ++i)
                to_rtn.batch_enque[i] = 128;

            for (int i = half_threads; i < num_threads; ++i)
                to_rtn.batch_deque[i] = 128;

            if (num_threads % 2 != 0)
                to_rtn.batch_enque[half_threads - 1] = 128;

            break;
        }

        default:
        {
            std::cerr << "Wrong config_recipe" << std::endl;
            break;
        }
        }

        return to_rtn;
    }
};

struct alignas(64) Counter
{
    int total_operations{};
    int succeeded_push{};
    int succeeded_pop{};
    int total_push{};
    int total_pop{};
    value_t sum_of_pushed_values{};
    value_t sum_of_poped_values{};
    double time{};
};

void reset_counter(Counter &counter)
{
    counter.total_operations = 0;
    counter.succeeded_push = 0;
    counter.succeeded_pop = 0;
    counter.total_push = 0;
    counter.total_pop = 0;
    counter.sum_of_pushed_values = 0;
    counter.sum_of_poped_values = 0;
    counter.time = 0.0;
}

struct Results
{
    double avg_time{};

    int total_n_operations{};
    int total_succeded_enqueues{};
    int total_succeded_dequeues{};

    int total_enqueues{};
    int total_dequeues{};
};

void update_results(Results &res, std::vector<Counter> const &counters)
{
    res.total_n_operations = std::accumulate(
        counters.begin(), counters.end(), 0,
        [](int sum, Counter const &c) { return sum + c.total_operations; });

    res.total_succeded_enqueues = std::accumulate(
        counters.begin(), counters.end(), 0,
        [](int sum, Counter const &c) { return sum + c.succeeded_push; });

    res.total_succeded_dequeues = std::accumulate(
        counters.begin(), counters.end(), 0,
        [](int sum, Counter const &c) { return sum + c.succeeded_pop; });

    res.total_enqueues = std::accumulate(counters.begin(), counters.end(), 0,
                                         [](int sum, Counter const &c)
                                         { return sum + c.total_push; });

    res.total_dequeues = std::accumulate(counters.begin(), counters.end(), 0,
                                         [](int sum, Counter const &c)
                                         { return sum + c.total_pop; });

    double total_time = std::accumulate(counters.begin(), counters.end(), 0.0,
                                        [](double sum, Counter const &c)
                                        { return sum + c.time; });
    res.avg_time = counters.empty() ? 0.0 : total_time / counters.size();
}

void calc_results(Results &res, Config const &config)
{
    // std::cout << "Implement me. Do not forget !!!" << std::endl;
    // Do some gloabal avergaing
}

class Benchmark
{
  private:
    Config config;
    std::vector<Counter> counters; // for each thread one
    Results results;
    bool verify_correctness(std::vector<Counter> const &counters,
                            value_t leftovers)
    {
        value_t total_pushed = 0;
        value_t total_popped = 0;

        for (auto const &c : counters)
        {
            total_pushed += c.sum_of_pushed_values;
            total_popped += c.sum_of_poped_values;
        }

        std::cout << "(pushed_val, poped_val, leftovers_val)= " << total_pushed
                  << " " << total_popped << " " << leftovers << std::endl;

        return total_pushed == (total_popped + leftovers);
    }

    std::vector<value_t> generate_batch_of_elements(int N, std::mt19937 &rng)
    {
        std::vector<value_t> to_rtn(N);
        // to_rtn.reserve(N);

        std::iota(to_rtn.begin(), to_rtn.end(), static_cast<value_t>(0));
        std::shuffle(to_rtn.begin(), to_rtn.end(), rng);
        return to_rtn;
    };

  public:
    Benchmark(Config &&cfg) : config(std::move(cfg))
    {
        if (!config.is_config_correct())
        {
            std::cerr << " I refuse to continue with this sick Job. Bye !"
                      << std::endl;
            std::abort();
        }
    };

    void run(BaseQueue &queue)
    {
        std::mt19937 global_rng(config.seed);

        counters.resize(config.num_threads);

        for (int i = 0; i < config.repetitions; i++)
        {
            // reset_counters();
#pragma omp parallel num_threads(config.num_threads)
            {
                int thread_id = omp_get_thread_num();
                Counter &l_counter = counters[thread_id];
                reset_counter(l_counter);

                int enqueue_batch_size = config.batch_enque[thread_id];
                int dequeue_batch_size = config.batch_deque[thread_id];
                std::mt19937 thread_rng(config.seed + thread_id + 1);

                double t_start = omp_get_wtime();
                while (omp_get_wtime() - t_start < config.max_time_in_s)
                {
                    std::vector<value_t> push_elements =
                        generate_batch_of_elements(enqueue_batch_size,
                                                   thread_rng);

                    for (int i = 0; i < enqueue_batch_size; i++)
                    {
                        value_t tmp = push_elements[i];

                        if (queue.push(tmp))
                        {
                            l_counter.sum_of_pushed_values += tmp;
                            l_counter.succeeded_push++;
                        };
                        l_counter.total_push++;
                    }

                    std::vector<value_t> poped_elements;
                    poped_elements.reserve(dequeue_batch_size);

                    for (int i = 0; i < dequeue_batch_size; i++)
                    {
                        // pop element and append locally
                        value_t tmp = queue.pop();
                        // aka if it did not fail
                        if (tmp != generics::empty_val)
                        {
                            l_counter.sum_of_poped_values += tmp;
                            l_counter.succeeded_pop++;
                        }
                        l_counter.total_pop++;
                    }
                }
                l_counter.total_operations =
                    l_counter.total_pop + l_counter.total_push;

                double t_end = omp_get_wtime();

                l_counter.time += t_end - t_start;

            } // End parallel
            #pragma omp barrier  // optional but good
            int leftovers = 0;
            int small_counter = 0;
            std::cout << "My empty val" << generics::empty_val << std::endl;
            std::cout << "Size of queue before counting leftovers "
                      << queue.get_size() << std::endl;

            while (true)
            {
                value_t tmp = queue.pop();
                if (tmp != generics::empty_val)
                {
                    leftovers += tmp;
                }
                else
                {
                    break;
                }
                small_counter++;
            }
            std::cout << "Size of queue at the end: " << queue.get_size()
                      << std::endl;
            std::cout << " I tried to pop at the end " << small_counter
                      << std::endl;

            if (!verify_correctness(counters, leftovers))
                std::cerr << "Something went terribly wrong" << std::endl;

            update_results(results, counters);
            std::cout << "\n\n";
        } // End for loop repetition

        calc_results(results, config);
    }

    void print_results() const
    {
        std::cout << "Results:\n";
        std::cout << "  Average time: " << results.avg_time << "\n";
        std::cout << "  Total operations: " << results.total_n_operations
                  << "\n";
        std::cout << "  Total succeeded enqueues: "
                  << results.total_succeded_enqueues << "\n";
        std::cout << "  Total succeeded dequeues: "
                  << results.total_succeded_dequeues << "\n";
        std::cout << "  Total enqueues: " << results.total_enqueues << "\n";
        std::cout << "  Total dequeues: " << results.total_dequeues << "\n";
    };
    void save_results(std::filesystem::path const &output) const {};
};