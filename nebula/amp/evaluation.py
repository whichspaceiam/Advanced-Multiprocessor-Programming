# import argparse
import pandas as pd
import matplotlib.pyplot as plt
# import numpy as np
from pathlib import Path

############################## PLOT FORMATTING ################################

plt.rc("figure", autolayout=True)
plt.rc("mathtext", default="regular")
plt.rc("axes", linewidth=1.5)

plt.rc("xtick", labelsize=20)
plt.rc("ytick", labelsize=20)

# Use rcParams for tick width and size
plt.rcParams["xtick.major.width"] = 1.5
plt.rcParams["xtick.minor.width"] = 1.5
plt.rcParams["ytick.major.width"] = 1.5
plt.rcParams["ytick.minor.width"] = 1.5

plt.rcParams["xtick.major.size"] = 6
plt.rcParams["xtick.minor.size"] = 3
plt.rcParams["ytick.major.size"] = 6
plt.rcParams["ytick.minor.size"] = 3

# Activate LaTeX style if supported
plt.rc("text", usetex=False)
plt.rc("font", family="serif")

############################## FUNCTIONS #######################################


def main(csv_file: Path, plot_dir: Path):
    df = pd.read_csv(csv_file)
    df["t_eff"] = df["avg_time"] - df["avg_timeout"]

    # Throughputs
    df["throughput_ops_s"] = df["operations"] / df["t_eff"]
    df["throughput_enq_s"] = df["s_enq"] / df["t_eff"]
    df["throughput_deq_s"] = df["s_deq"] / df["t_eff"]

    # Throughput speedup vs 1 thread (per queue type)
    base = df[df["n_threads"] == 1][["name", "throughput_ops_s"]].rename(
        columns={"throughput_ops_s": "throughput_1t"}
    )
    df = df.merge(base, on="name", how="left")

    df["speedup_throughput"] = df["throughput_ops_s"] / df["throughput_1t"]
    df["efficiency"] = df["speedup_throughput"] / df["n_threads"]

    # Pretty table
    display_cols = [
        "name",
        "n_threads",
        "operations",
        "t_eff",
        "throughput_ops_s",
        "speedup_throughput",
        "efficiency",
        "throughput_enq_s",
        "throughput_deq_s",
    ]
    df_out = df[display_cols].sort_values(["name", "n_threads"]).copy()

    # nicer numbers for notebook display
    df_out["throughput_ops_s"] = df_out["throughput_ops_s"].round(0).astype(int)
    df_out["throughput_enq_s"] = df_out["throughput_enq_s"].round(0).astype(int)
    df_out["throughput_deq_s"] = df_out["throughput_deq_s"].round(0).astype(int)
    df_out["speedup_throughput"] = df_out["speedup_throughput"].round(3)
    df_out["efficiency"] = df_out["efficiency"].round(3)
    df_out["t_eff"] = df_out["t_eff"].round(6)
    thr = df.pivot(index="n_threads", columns="name", values="throughput_ops_s")
    spd = df.pivot(index="n_threads", columns="name", values="speedup_throughput")
    eff = df.pivot(index="n_threads", columns="name", values="efficiency")
    thr.plot(marker="o")
    plt.title("Throughput (ops/s)")
    plt.xlabel("Threads")
    plt.ylabel("ops/s")
    plt.grid(True, alpha=0.3)
    plt.savefig(plot_dir / "throughput.png")

    spd.plot(marker="o")
    plt.title("Throughput speedup vs 1 thread")
    plt.xlabel("Threads")
    plt.ylabel("speedup")
    plt.axhline(1.0, color="k", lw=1, alpha=0.3)
    plt.grid(True, alpha=0.3)
    plt.savefig(plot_dir / "speedup.png")

    eff.plot(marker="o")
    plt.title("Efficiency = speedup / threads")
    plt.xlabel("Threads")
    plt.ylabel("efficiency")
    plt.axhline(1.0, color="k", lw=1, alpha=0.3)
    plt.grid(True, alpha=0.3)
    plt.savefig(plot_dir / "efficiency.png")
    plt.show()


if __name__ == "__main__":
    SMALL_BENCH_DATA = Path("./data/results_small_bench.csv")
    PLOT_DIR = Path("./plots")
    main(SMALL_BENCH_DATA, PLOT_DIR)
