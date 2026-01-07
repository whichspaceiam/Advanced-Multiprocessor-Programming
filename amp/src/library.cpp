#include "benchmark.hpp"
#include "fine_lock.hpp"
#include "lock_free_aba.hpp"
#include "timer.hpp"
#include <iostream>
#include <map>
#include <memory>

struct Arguments
{
    std::string config_recipe{"balanced"};
    int max_time{1};
    int sets{0};
    int seed{42};
    int repetitions{1};
    int n_threads{1};
    std::string type{"sequential"};
    bool is_safe_run{false};
    bool cache_checks{false};
    bool print_header{false};

    bool are_valid() const
    {
        // Validate config_recipe
        if (config_recipe != "balanced" && config_recipe != "thread")
        {
            std::cerr << "Error: config_recipe must be 'balanced' or 'thread', got: "
                      << config_recipe << std::endl;
            return false;
        }

        // Validate type
        if (type != "global_lock" && type != "fine_lock" && type != "lock_free" && type !="sequential")
        {
            std::cerr << "Error: type must be 'global_lock', 'fine_lock', or 'lock_free', got: "
                      << type << std::endl;
            return false;
        }

        // Validate positive integers
        if (max_time < 0)
        {
            std::cerr << "Error: max_time must be >= 0, got: " << max_time << std::endl;
            return false;
        }
        if (sets < 0)
        {
            std::cerr << "Error: sets must be >= 0, got: " << max_time << std::endl;
            return false;
        }

        if (!((sets == 0) ^ (max_time == 0)))
        {
            std::cerr << " You need to specify max time or sets based benchmark. That means either time or sets are non zero and the other parameter is zero." << "We got for sets" << sets << "  and max_time  " << max_time << std::endl;
            return false;
        }

        if (sets != 0 && is_safe_run)
        {
            std::cerr << " Safe run can only run in a time based benchmark. Use e.g --max_time 1 --sets 0" << std::endl;
            return false;
        }

        if (is_safe_run && cache_checks)
        {
            std::cerr << "Safe run and cahce checks cannot be used at the same time. First do a benchmark with safe run and then cache checks. The reason is that safe run introduces overheads that will produce false resutls for the cache hits." << std::endl;
        }

        if (repetitions <= 0)
        {
            std::cerr << "Error: repetitions must be > 0, got: " << repetitions << std::endl;
            return false;
        }

        if (n_threads <= 0)
        {
            std::cerr << "Error: n_threads must be > 0, got: " << n_threads << std::endl;
            return false;
        }

        // Validate cache_checks constraint
        if (cache_checks && type != "lock_free")
        {
            std::cerr << "Error: cache_checks can only be used with type='lock_free'" << std::endl;
            return false;
        }

        return true;
    }
};
Arguments parse(int argc, char *argv[])
{
    Arguments args; // Starts with default values

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        auto get_next_value = [&]() -> std::string
        {
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                return argv[++i];
            }
            return "";
        };

        try
        {
            if (arg == "--config_recipe")
            {
                args.config_recipe = get_next_value();
            }
            else if (arg == "--max_time")
            {
                std::string val = get_next_value();
                if (!val.empty())
                {
                    args.max_time = std::stoi(val);
                }
            }
            else if (arg == "--seed")
            {
                std::string val = get_next_value();
                if (!val.empty())
                {
                    args.seed = std::stoi(val);
                }
            }
            else if (arg == "--repetitions")
            {
                std::string val = get_next_value();
                if (!val.empty())
                {
                    args.repetitions = std::stoi(val);
                }
            }
            else if (arg == "--sets")
            {
                std::string val = get_next_value();
                if (!val.empty())
                {
                    args.sets = std::stoi(val);
                }
            }
            else if (arg == "--n_threads")
            {
                std::string val = get_next_value();
                if (!val.empty()) // FIXED: Added empty check
                {
                    args.n_threads = std::stoi(val);
                }
            }
            else if (arg == "--type")
            {
                args.type = get_next_value();
            }
            else if (arg == "--is_safe_run")
            {
                args.is_safe_run = true;
            }
            else if (arg == "--cache_checks")
            {
                args.cache_checks = true;
            }
            else if (arg == "--print_header")
            {
                args.print_header = true;
            }
            else
            {
                std::cerr << "Warning: Unknown flag '" << arg << "'" << std::endl;
            }
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Error: Invalid value for flag '" << arg << "'" << std::endl;
            // Continue parsing other arguments
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Error: Value out of range for flag '" << arg << "'" << std::endl;
            // Continue parsing other arguments
        }
    }

    return args;
}

int main(int argc, char *argv[])
{

    auto args = parse(argc, argv);

    if (!args.are_valid())
    {
        std::cout << "Arguments are not valid" << std::endl;
        return 1;
    }

    // FIXED: Safe map access with validation
    std::map<std::string, ConfigRecipe> config_recipe_map{
        {"balanced", ConfigRecipe::Balanced},
        {"thread", ConfigRecipe::ThreadSpecific}};

    // This is now safe because are_valid() checks config_recipe
    Config config = ConfigFactory{
        args.n_threads,
        args.repetitions,
        args.max_time,
        args.sets,
        args.seed,
        config_recipe_map[args.config_recipe]}();

    Benchmark benchmark{std::move(config)};

    std::unique_ptr<BaseQueue> queue;
    if (args.type == "global_lock")
    {
        queue = std::make_unique<global_lock::Queue>();
    }
    else if (args.type == "fine_lock")
    {
        queue = std::make_unique<fine_lock::Queue>();
    }
    else if (args.type == "lock_free")
    {
        queue = std::make_unique<lock_free_aba::Queue>();
    }
    else if (args.type == "sequential")
    {
        // std::cout<<"Sequential mode"<<args.n_threads<<std::endl;
        if(args.n_threads!=1){
            std::cerr<<" n_threads>1 in sequential benchmark !!!"<<std::endl;
            std::abort();
        }
        queue = std::make_unique<seq::Queue>();
    }
    else
    {
        // FIXED: This branch is now unreachable due to are_valid() check
        // But keeping it for defensive programming
        std::cerr << "Invalid queue type. Available: global_lock, fine_lock, lock_free" << std::endl;
        return 1; // FIXED: Added return to prevent nullptr dereference
    }

    // FIXED: Additional safety check (defensive programming)
    if (!queue)
    {
        std::cerr << "Failed to create queue" << std::endl;
        return 1;
    }

    if (args.is_safe_run)
    {

        benchmark.run_safe(*queue);
    }
    else if (args.cache_checks) // Run cache line hits benchmark
    {
        auto *lock_free_queue = dynamic_cast<lock_free_aba::Queue *>(queue.get());
        assert(lock_free_queue != nullptr && "cache_checks requires lock_free_aba::Queue");
        std::cout << " Starting running cache checks" << std::endl;
        auto total = benchmark.run_fast_cache_checks(*lock_free_queue);
        std::cout << "Total cachec success: " << total.success << " \n Total cache failures: " << total.failures << std::endl;
    } else if( args.max_time != 0)
    {
        benchmark.run_fast(*queue);
    }
    else if(args.sets != 0){
        benchmark.run_sets(*queue);
    }
    else {
        std::cerr<< " For devs .Benchmark does not make sense. Please add guards in validation "<<std::endl;
    }
    benchmark.print_csv(args.type, args.print_header);
    return 0;
}
