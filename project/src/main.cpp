#include "benchmark.hpp"
#include "timer.hpp"
#include <iostream>

int main()
{
    ConfigRecipe recipe = ConfigRecipe::ThreadSpecific;
    int n_threads = 10;
    int max_time = 1;
    int seed = 42;
    int repetitions = 2;
    Config config =
        ConfigFactory{n_threads, repetitions, max_time, seed, recipe}();

    if (config.is_config_correct())
    {
        std::cout << "Config is fucking correct" << std::endl;
    }

    Benchmark benchmark{std::move(config)};



    std::cout << "Hello master of distaster" << std::endl;
    Timer t;
    global_lock::ConcurrentQueue queue;
    benchmark.run(queue);
    benchmark.print_results();
    // t.checkpoint();
    // auto res = t.get_laps();
    // for (auto timing : res)
    //     std::cout << timing << " [ms], ";
    // std::cout << std::endl;
}