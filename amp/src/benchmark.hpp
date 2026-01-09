#pragma once
#include "lock_guard.hpp"
#include "sequential.hpp"
#include "lock_free_aba.hpp"
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
    UpperHalf,
    EvenOdd,
    OneToAll
};

struct Config
{
    size_t num_threads;
    size_t repetitions;
    int max_time_in_s;
    size_t sets;
    int seed;
    std::vector<int> batch_enque; // per thread batch
    std::vector<int> batch_deque; // per thread batch

    bool is_config_correct()
    {
        if (batch_deque.size() != num_threads)
            return false;
        if (batch_enque.size() != num_threads)
            return false;

        // int sum_pushed_batches =
        //     std::accumulate(batch_enque.begin(), batch_enque.end(), 0);
        // int sum_poped_batches =
        //     std::accumulate(batch_deque.begin(), batch_deque.end(), 0);

        // if (sum_poped_batches != sum_pushed_batches)
        // {
            
        //     std::cout << " Not balanced batches are declared" << std::endl;
        //     return false;
        // }

        if (max_time_in_s > 100)
        {
            std::cout << "Time specification is not reasonable" << std::endl;
            return false;
        }

        if (sets == 0 && max_time_in_s == 0)
        {
            std::cout << "Either you need to specify a time based benchmark or a set based benchmark" << std::endl;
            return false;
        }

        if (repetitions > 100)
        {
            std::cout << "Repetition specification seems irresonable"
                      << std::endl;
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
    int sets;
    int seed;
    ConfigRecipe config_recipe;

public:
    ConfigFactory(int num_threads, int repetitions, int max_time_in_s, int sets, int seed,
                  ConfigRecipe config_recipe)
        : num_threads(num_threads), repetitions(repetitions),
          max_time_in_s(max_time_in_s), sets(sets), seed(seed), config_recipe(config_recipe)
    {
    }

    Config operator()(size_t batch_size)
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
            to_rtn.batch_enque.resize(num_threads, batch_size);
            to_rtn.batch_deque.resize(num_threads, batch_size);
            break;
        }

        case ConfigRecipe::UpperHalf:
        {
            to_rtn.batch_enque.resize(num_threads, 0);
            to_rtn.batch_deque.resize(num_threads, 0);

            int half_threads = num_threads / 2;

            for (int i = 0; i < half_threads; ++i)
                to_rtn.batch_enque[i] = batch_size;

            for (int i = half_threads; i < num_threads; ++i)
                to_rtn.batch_deque[i] = batch_size;

            if (num_threads % 2 != 0)
                to_rtn.batch_enque[half_threads - 1] = batch_size;

            break;
        }

        case ConfigRecipe::OneToAll:{
            to_rtn.batch_enque.resize(num_threads, 0);
            to_rtn.batch_deque.resize(num_threads, 0);

            to_rtn.batch_enque[0] = batch_size;
            for (int i = 1; i<num_threads; ++i){
                to_rtn.batch_deque[i] = batch_size;
            }
            
            break;
        }
        case ConfigRecipe::EvenOdd:{
            to_rtn.batch_enque.resize(num_threads, 0);
            to_rtn.batch_deque.resize(num_threads, 0);

            for (int i = 0; i < num_threads; i+=2)
                to_rtn.batch_enque[i] = batch_size;

            for (int i = 1; i<num_threads; i+=2)
                to_rtn.batch_deque[i] = batch_size;
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

struct Counter
{
    size_t total_operations{};
    size_t succeeded_push{};
    size_t succeeded_pop{};
    size_t total_push{};
    size_t total_pop{};
    value_t sum_of_pushed_values{};
    value_t sum_of_poped_values{};
    double time{};
    double timeout{};
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
    counter.timeout = 0.0;
}

struct Results
{
    double avg_time{};
    double avg_timeout{};

    size_t total_n_operations{};
    size_t total_succeded_enqueues{};
    size_t total_succeded_dequeues{};

    size_t total_enqueues{};
    size_t total_dequeues{};
};

void update_results(Results &res, std::vector<Counter> const &counters)
{
    res.total_n_operations = std::accumulate(
        counters.begin(), counters.end(), 0,
        [](size_t sum, Counter const &c)
        { return sum + c.total_operations; });

    res.total_succeded_enqueues = std::accumulate(
        counters.begin(), counters.end(), 0,
        [](size_t sum, Counter const &c)
        { return sum + c.succeeded_push; });

    res.total_succeded_dequeues = std::accumulate(
        counters.begin(), counters.end(), 0,
        [](size_t sum, Counter const &c)
        { return sum + c.succeeded_pop; });

    res.total_enqueues = std::accumulate(counters.begin(), counters.end(), 0,
                                         [](size_t sum, Counter const &c)
                                         { return sum + c.total_push; });

    res.total_dequeues = std::accumulate(counters.begin(), counters.end(), 0,
                                         [](size_t sum, Counter const &c)
                                         { return sum + c.total_pop; });

    double total_time = std::accumulate(counters.begin(), counters.end(), 0.0,
                                        [](double sum, Counter const &c)
                                        { return sum + c.time; });

    double total_timeout = std::accumulate(counters.begin(), counters.end(), 0.0,
                                           [](double sum, Counter const &c)
                                           { return sum + c.timeout; });
    res.avg_time = counters.empty() ? 0.0 : total_time / counters.size();
    res.avg_timeout = counters.empty() ? 0.0 : total_timeout / counters.size();
}

void calc_results(Results &res, Config const &config)
{
    int repetitions = config.repetitions;

    res.avg_time /= repetitions;
    res.avg_timeout /= repetitions;
    res.total_n_operations /= repetitions;
    res.total_dequeues /= repetitions;
    res.total_enqueues /= repetitions;
    res.total_succeded_dequeues /= repetitions;
    res.total_succeded_enqueues /= repetitions;
}

class Benchmark
{
private:
    Config config;
    std::vector<Counter> counters; // for each thread one
    Results results{};
    size_t _prefill{};
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

        return total_pushed == (total_popped + leftovers);
    }

    std::vector<value_t> generate_batch_of_elements(uint N, std::mt19937 &rng)
    {
        std::vector<value_t> to_rtn(N);
        std::iota(to_rtn.begin(), to_rtn.end(), static_cast<value_t>(0));
        std::shuffle(to_rtn.begin(), to_rtn.end(), rng);
        return to_rtn;
    };

public:
    Benchmark(Config &&cfg, size_t prefill) : config(std::move(cfg)), _prefill(prefill)
    {
        if (!config.is_config_correct())
        {
            std::cerr << " I refuse to continue with this sick Job. Bye !"
                      << std::endl;
            std::abort();
        }
    };

    void run_safe(BaseQueue &queue)
    {
        std::mt19937 global_rng(config.seed);

        counters.resize(config.num_threads);

        for (size_t i = 0; i < config.repetitions; i++)
        {
#pragma omp parallel num_threads(config.num_threads)
            {
                uint thread_id = omp_get_thread_num();
                Counter &l_counter = counters[thread_id];
                reset_counter(l_counter);

                uint enqueue_batch_size = config.batch_enque[thread_id];
                uint dequeue_batch_size = config.batch_deque[thread_id];
                std::mt19937 thread_rng(config.seed + thread_id + 1);
                double timeout = 0.0;

                double t_start = omp_get_wtime();
                while (omp_get_wtime() - t_start < config.max_time_in_s)
                {
                    double t0 = omp_get_wtime();
                    std::vector<value_t> push_elements =
                        generate_batch_of_elements(enqueue_batch_size,
                                                   thread_rng);
                    double t1 = omp_get_wtime();
                    timeout += t1 - t0;

                    for (size_t i = 0; i < enqueue_batch_size; i++)
                    {
                        value_t tmp = push_elements[i];

                        if (queue.push(tmp))
                        {
                            l_counter.sum_of_pushed_values += tmp;
                            l_counter.succeeded_push++;
                        };
                        l_counter.total_push++;
                    }

                    t0 = omp_get_wtime();
                    std::vector<value_t> poped_elements;
                    poped_elements.reserve(dequeue_batch_size);
                    t1 = omp_get_wtime();
                    timeout += t1 - t0;

                    for (size_t i = 0; i < dequeue_batch_size; i++)
                    {
                        value_t tmp = queue.pop();
                        if (tmp != generics::empty_val)
                        {
                            l_counter.sum_of_poped_values += tmp;
                            l_counter.succeeded_pop++;
                        }
                        l_counter.total_pop++;
                    }
                }
                double t_end = omp_get_wtime();
                l_counter.total_operations =
                    l_counter.total_pop + l_counter.total_push;

                l_counter.time += t_end - t_start;
                l_counter.timeout += timeout;

            } // End parallel
#pragma omp barrier
            value_t leftovers = count_leftovers_n_empty(queue);
            if (!verify_correctness(counters, leftovers))
                std::cerr << "Something went terribly wrong" << std::endl;

            update_results(results, counters);
        } // End for loop repetition

        calc_results(results, config);
    }

    value_t count_leftovers_n_empty(BaseQueue &queue)
    {
        value_t leftovers = 0;

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
        }

        return leftovers;
    }

    void run_fast(BaseQueue &queue)
    {
        std::mt19937 global_rng(config.seed);

        counters.resize(config.num_threads);

        // Prefill
        for (int i = 0; i < _prefill; i++)
        {
            queue.push(i);
        }

        for (size_t i = 0; i < config.repetitions; i++)
        {
#pragma omp parallel num_threads(config.num_threads)
            {
                uint thread_id = omp_get_thread_num();
                Counter &l_counter = counters[thread_id];
                reset_counter(l_counter);

                uint enqueue_batch_size = config.batch_enque[thread_id];
                uint dequeue_batch_size = config.batch_deque[thread_id];
                std::mt19937 thread_rng(config.seed + thread_id + 1);
                double timeout = 0.0;

                std::vector<value_t> push_elements =
                    generate_batch_of_elements(enqueue_batch_size,
                                               thread_rng);
#pragma omp barrier
                double t_start = omp_get_wtime();
                while (omp_get_wtime() - t_start < config.max_time_in_s)
                {

                    for (size_t i = 0; i < enqueue_batch_size; i++)
                    {
                        if (queue.push(push_elements[i]))
                            l_counter.succeeded_push++;
                    }
                    l_counter.total_push += enqueue_batch_size;

                    for (size_t i = 0; i < dequeue_batch_size; i++)
                    {
                        if (queue.pop() != generics::empty_val)
                            l_counter.succeeded_pop++;
                    }
                    l_counter.total_pop += dequeue_batch_size;
                }
                double t_end = omp_get_wtime();
#pragma omp barrier

                l_counter.total_operations =
                    l_counter.total_pop + l_counter.total_push;
                l_counter.time += t_end - t_start;
                l_counter.timeout += timeout;

            } // End parallel

            update_results(results, counters);
        } // End for loop repetition

        calc_results(results, config);
    }

    void run_sets(BaseQueue &queue)
    {
        std::mt19937 global_rng(config.seed);
        counters.resize(config.num_threads);

        for (size_t i = 0; i < config.repetitions; i++)
        {
#pragma omp parallel num_threads(config.num_threads)
            {
                uint thread_id = omp_get_thread_num();
                Counter &l_counter = counters[thread_id];
                reset_counter(l_counter);

                uint enqueue_batch_size = config.batch_enque[thread_id];
                uint dequeue_batch_size = config.batch_deque[thread_id];
                std::mt19937 thread_rng(config.seed + thread_id + 1);
                double timeout = 0.0;

                std::vector<value_t> push_elements =
                    generate_batch_of_elements(enqueue_batch_size,
                                               thread_rng);
                assert(config.sets != 0);
#pragma omp barrier
                double t_start = omp_get_wtime();
                for (size_t i{0}; i < config.sets; ++i)
                {

                    for (size_t i = 0; i < enqueue_batch_size; i++)
                    {
                        queue.push(push_elements[i]);
                    }
                    l_counter.total_push += enqueue_batch_size;
                    l_counter.succeeded_push += enqueue_batch_size;

                    for (size_t i = 0; i < dequeue_batch_size; i++)
                    {
                        queue.pop();
                    }
                    l_counter.total_pop += dequeue_batch_size;
                    l_counter.succeeded_pop += enqueue_batch_size;
                }
                double t_end = omp_get_wtime();
#pragma omp barrier

                l_counter.total_operations =
                    l_counter.total_pop + l_counter.total_push;
                l_counter.time += t_end - t_start;
                l_counter.timeout += timeout;

            } // End parallel

            update_results(results, counters);
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

    void print_csv(std::string const &name, bool header = false) const
    {
        if (header)
        {
            std::cout << "name,n_threads,avg_time,avg_timeout,operations,s_enq,s_deq,enq,deq\n";
        }

        std::cout << name << ",";
        std::cout << config.num_threads << ",";
        std::cout << results.avg_time << ",";
        std::cout << results.avg_timeout << ",";
        std::cout << results.total_n_operations << ",";
        std::cout << results.total_succeded_enqueues << ",";
        std::cout << results.total_succeded_dequeues << ",";
        std::cout << results.total_enqueues << ",";
        std::cout << results.total_dequeues;
        std::cout << std::endl;
    };
    // void save_results(std::filesystem::path const &output) const {};
};