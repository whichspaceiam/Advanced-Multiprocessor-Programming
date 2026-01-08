"""
This file is the main Python driver for the queue benchmark suite.

It orchestrates large-scale benchmark experiments for different
queue implementations exposed via the C++/pybind11 module `benchmark_wrapper`.

It defines:
- the tested queue types (sequential, global lock, fine-grained lock, lock-free)
- different thread counts
- different batch sizes
- different workload distributions (balanced, single producer, split roles, even/odd)

For each configuration, it:
- constructs a benchmark configuration
- executes the benchmark via the C++ backend
- collects aggregated performance metrics
- writes the results into a CSV file for later analysis and plotting
"""

import benchmark_wrapper as bw
import itertools
import csv
from pathlib import Path


# --- Queue-Typen ---
queue_types = ["sequential", "global_lock", "fine_lock", "lock_free"]

# --- Thread-Zahlen ---
threads_list = [1, 2, 8, 10, 20, 32, 45, 64]

# --- Batch-Größen ---
batch_sizes = [1, 1000]

# --- Thread-Strategien ---
# a) balanced, b) one enqueuer, c) half enqueuer, d) even/odd
strategies = ["balanced", "one_enqueuer", "half_enqueuer", "even_odd"]

# --- Wiederholungen ---
repetitions = 10

# --- Sequenzielle Queue Run Zeit ---
seq_times = [1, 5]

# --- Ergebnis CSV vorbereiten ---
out_file = Path("benchmark_large_results.csv")
with out_file.open("w", newline="") as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow([
        "queue_type", "threads", "batch_enque", "batch_deque",
        "strategy", "avg_time", "avg_timeout", "total_ops",
        "succ_enq", "succ_deq", "total_enq", "total_deq"
    ])

# --- Hilfsfunktion für Batch-Layout ---
def generate_batch_layout(num_threads, batch_size, strategy):
    enq = [batch_size] * num_threads
    deq = [batch_size] * num_threads

    if strategy == "balanced":
        return enq, deq
    elif strategy == "one_enqueuer":
        enq = [0] * num_threads
        enq[0] = batch_size * num_threads  # Alle enqueues auf 1 Thread
        deq = [batch_size] * num_threads
        return enq, deq
    elif strategy == "half_enqueuer":
        enq = [0] * num_threads
        deq = [0] * num_threads
        half = num_threads // 2
        for i in range(half):
            enq[i] = batch_size
        for i in range(half, num_threads):
            deq[i] = batch_size
        # Balance prüfen und korrigieren
        sum_enq = sum(enq)
        sum_deq = sum(deq)
        if sum_enq != sum_deq:
            enq[half-1] += (sum_deq - sum_enq)
        return enq, deq
    elif strategy == "even_odd":
        enq = [batch_size if i % 2 == 0 else 0 for i in range(num_threads)]
        deq = [batch_size if i % 2 == 1 else 0 for i in range(num_threads)]
        # Summe ausgleichen
        sum_enq = sum(enq)
        sum_deq = sum(deq)
        if sum_enq != sum_deq:
            enq[0] += (sum_deq - sum_enq)
        return enq, deq
    else:
        raise ValueError(f"Unknown strategy: {strategy}")
    
# --- Main Benchmark Loop ---
for queue_type in queue_types:
    if queue_type == "sequential":
        # Nur 1 Thread
        for time_sec in seq_times:
            for batch in batch_sizes:
                config = bw.Config()
                config.num_threads = 1
                config.max_time_in_s = time_sec
                config.repetitions = repetitions
                config.seed = 42
                config.batch_enque = [batch]
                config.batch_deque = [batch]

                bench = bw.Benchmark(config)
                bench.run_fast("sequential")
                results = bench.get_results()

                with out_file.open("a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerow([
                        "sequential", 1, batch, batch, "seq_run",
                        results.avg_time, results.avg_timeout,
                        results.total_n_operations,
                        results.total_succeded_enqueues,
                        results.total_succeded_dequeues,
                        results.total_enqueues,
                        results.total_dequeues
                    ])
    else:
        # Concurrent Queues
        for num_threads, batch in itertools.product(threads_list, batch_sizes):
            for strategy in strategies:
                config = bw.Config()
                config.num_threads = num_threads
                config.max_time_in_s = 1  # Time-based benchmark
                config.repetitions = repetitions
                enq, deq = generate_batch_layout(num_threads, batch, strategy)
                config.seed = 42
                config.batch_enque = enq
                config.batch_deque = deq

                bench = bw.Benchmark(config)
                bench.run_fast(queue_type)
                results = bench.get_results()

                # validity check
                # Validitätscheck
                assert results.total_succeded_enqueues == results.total_succeded_dequeues, (
                    f"Queue results invalid: {queue_type}, threads={num_threads}, strategy={strategy}, "
                    f"succ_enq={results.total_succeded_enqueues}, succ_deq={results.total_succeded_dequeues}"
                )

                with out_file.open("a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerow([
                        queue_type, num_threads, enq, deq, strategy,
                        results.avg_time, results.avg_timeout,
                        results.total_n_operations,
                        results.total_succeded_enqueues,
                        results.total_succeded_dequeues,
                        results.total_enqueues,
                        results.total_dequeues
                    ])

print("Benchmark complete! Results saved to benchmark_large_results.csv")
