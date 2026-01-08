/*
This file defines the Python bindings for the C++ benchmark framework
using pybind11.

Purpose:
--------
It exposes the core C++ classes (Config, Benchmark, Results) to Python
so that benchmarks can be configured and executed from Python scripts.

Specifically, this file:
- maps C++ configuration structures to Python-accessible classes
- exposes benchmark execution methods (run_fast, run_safe, run_sets)
- provides access to benchmark results
- acts as the bridge between high-performance C++ code and flexible Python control

Why this file is necessary:
---------------------------
The benchmark core is implemented in C++ for performance, thread control,
and low-level synchronization. Python is used only as a high-level
orchestration layer.

Without this wrapper, Python could not directly call into the benchmark
logic nor retrieve structured performance results.
*/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "benchmark.hpp"
#include "fine_lock.hpp"
#include "lock_free_aba.hpp"
#include "sequential.hpp"
#include "lock_guard.hpp"


namespace py = pybind11;

PYBIND11_MODULE(benchmark_wrapper, m) {
    // --- Enums ---
    py::enum_<ConfigRecipe>(m, "ConfigRecipe")
        .value("Balanced", ConfigRecipe::Balanced)
        .value("ThreadSpecific", ConfigRecipe::ThreadSpecific);

    // --- Config class ---
    py::class_<Config>(m, "Config")
        .def(py::init<>())
        .def_readwrite("num_threads", &Config::num_threads)
        .def_readwrite("repetitions", &Config::repetitions)
        .def_readwrite("max_time_in_s", &Config::max_time_in_s)
        .def_readwrite("sets", &Config::sets)
        .def_readwrite("seed", &Config::seed)
        .def_readwrite("batch_enque", &Config::batch_enque)
        .def_readwrite("batch_deque", &Config::batch_deque)
        .def("is_config_correct", &Config::is_config_correct);

    // --- Results struct ---
    py::class_<Results>(m, "Results")
        .def_readonly("avg_time", &Results::avg_time)
        .def_readonly("avg_timeout", &Results::avg_timeout)
        .def_readonly("total_n_operations", &Results::total_n_operations)
        .def_readonly("total_succeded_enqueues", &Results::total_succeded_enqueues)
        .def_readonly("total_succeded_dequeues", &Results::total_succeded_dequeues)
        .def_readonly("total_enqueues", &Results::total_enqueues)
        .def_readonly("total_dequeues", &Results::total_dequeues);

    py::class_<Benchmark>(m, "Benchmark")
        .def(py::init<const Config&>())
        .def("run_safe", [](Benchmark &b, const std::string &type) {
            if (type == "global_lock") { global_lock::Queue q; b.run_safe(q); }
            else if (type == "fine_lock") { fine_lock::Queue q; b.run_safe(q); }
            else if (type == "lock_free") { lock_free_aba::Queue q; b.run_safe(q); }
            else if (type == "sequential") { seq::Queue q; b.run_safe(q); }
            else throw std::runtime_error("Unknown queue type");
        })
        .def("run_fast", [](Benchmark &b, const std::string &type) {
            if (type == "global_lock") { global_lock::Queue q; b.run_fast(q); }
            else if (type == "fine_lock") { fine_lock::Queue q; b.run_fast(q); }
            else if (type == "lock_free") { lock_free_aba::Queue q; b.run_fast(q); }
            else if (type == "sequential") { seq::Queue q; b.run_fast(q); }
            else throw std::runtime_error("Unknown queue type");
        })
        .def("run_sets", [](Benchmark &b, const std::string &type) {
            if (type == "global_lock") { global_lock::Queue q; b.run_sets(q); }
            else if (type == "fine_lock") { fine_lock::Queue q; b.run_sets(q); }
            else if (type == "lock_free") { lock_free_aba::Queue q; b.run_sets(q); }
            else if (type == "sequential") { seq::Queue q; b.run_sets(q); }
            else throw std::runtime_error("Unknown queue type");
        })
        .def("run_cache_checks", [](Benchmark &b) {
            lock_free_aba::Queue q;
            return b.run_fast_cache_checks(q);
        })
        .def("print_csv", &Benchmark::print_csv)  // <-- kein Semikolon hier
        .def("get_results", [](Benchmark &b) -> const Results& {
            return b.get_results_ref();
        });

}
