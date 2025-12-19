#include "benchmark.hpp"
#include "fine_lock.hpp"
#include "lock_free.hpp"
#include "lock_free_aba.hpp"
#include "timer.hpp"
#include <iostream>

int main()
{
    // ConfigRecipe recipe = ConfigRecipe::Balanced;
    ConfigRecipe recipe = ConfigRecipe::Balanced;
    // int n_threads = 1;
    int max_time = 1;
    int seed = 42;
    int repetitions = 1;

    
    // COnfig config{};
    // config.batch_enque = std::vector<>{1316}
    
    // if (!config.is_config_correct())
    // {
    //     std::cout << "Config is not correct ERRROR!" << std::endl;
    // }
    
    // {
    //     Config config =
    //         ConfigFactory{2, repetitions, max_time, seed, recipe}();
    //     Benchmark benchmark{std::move(config)};
    //     seq::Queue queue;
    //     benchmark.run(queue);
    //     benchmark.print_csv("seq", true);
    // }

    for (auto n_threads : std::vector<int>{1,2, 4, 8})
    {

        Config config =
            ConfigFactory{n_threads, repetitions, max_time, seed, recipe}();
        if (!config.is_config_correct())
        {
            std::cout << "Config is not correct ERRROR!" << std::endl;
            break;
        }
        {
            Config config_cp = config;
            Benchmark benchmark{std::move(config_cp)};
            global_lock::ConcurrentQueue queue;
            benchmark.run_fast(queue);
            benchmark.print_csv("glob", false);
        }
        {
            Config config_cp = config;
            Benchmark benchmark{std::move(config_cp)};
            finelock::Queue queue;
            benchmark.run_fast(queue);
            benchmark.print_csv("finelock", false);
        }
        {
            Config config_cp = config;
            Benchmark benchmark{std::move(config_cp)};
            lock_free_aba::Queue queue;
            benchmark.run_fast(queue);
            benchmark.print_csv("lockfree", false);
        }
    }
    // global_lock::ConcurrentQueue queue;
    // finelock::Queue queue;
    // lock_free_aba::Queue queue;

    // assert(finelock::empty_val == generics::empty_val);
    // std::cout<< " I experienced header tail problem :" <<queue.header_tail_condition<<std::endl;
    // std::cout<< " I found null header problem :" <<queue.null_header_cnt<<std::endl;
    // benchmark.print_results();
    // std::cout << "-----" << std::endl;

    // t.checkpoint();
    // auto res = t.get_laps();
    // for (auto timing : res)
    //     std::cout << timing << " [ms], ";
    // std::cout << std::endl;
}