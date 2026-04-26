#!/usr/bin/env python3
"""Plot scaling and Amdahl benchmark outputs.

Expected input format (scaling dat):
# threads time_mean time_std speedup efficiency amdahl_speedup
1 10.0 0.2 1.0 1.0 1.0
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import List


def load_scaling_dat(path: Path):
    threads: List[int] = []
    time_mean: List[float] = []
    time_std: List[float] = []
    speedup: List[float] = []
    efficiency: List[float] = []
    amdahl_speedup: List[float] = []

    with path.open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 5:
                continue

            threads.append(int(parts[0]))
            time_mean.append(float(parts[1]))
            time_std.append(float(parts[2]))
            speedup.append(float(parts[3]))
            efficiency.append(float(parts[4]))
            amdahl_speedup.append(float(parts[5]) if len(parts) >= 6 else float(parts[3]))

    return threads, time_mean, time_std, speedup, efficiency, amdahl_speedup


def plot_scaling(input_path: Path, output_path: Path, plt) -> None:
    threads, time_mean, time_std, speedup, efficiency, amdahl_speedup = load_scaling_dat(input_path)

    fig, axes = plt.subplots(1, 3, figsize=(16, 5), constrained_layout=True)
    fig.suptitle(f"Scaling analysis: {input_path.stem}", fontsize=14)

    axes[0].errorbar(threads, time_mean, yerr=time_std, fmt="o-", capsize=4, color="#1f77b4")
    axes[0].set_title("Time vs Threads")
    axes[0].set_xlabel("Threads")
    axes[0].set_ylabel("Time [s]")
    axes[0].grid(True, alpha=0.3)

    axes[1].plot(threads, speedup, "o-", color="#2ca02c", label="Measured")
    axes[1].plot(threads, amdahl_speedup, "s--", color="#d62728", label="Amdahl")
    axes[1].set_title("Speedup")
    axes[1].set_xlabel("Threads")
    axes[1].set_ylabel("Speedup")
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()

    axes[2].plot(threads, efficiency, "o-", color="#ff7f0e")
    axes[2].set_title("Efficiency")
    axes[2].set_xlabel("Threads")
    axes[2].set_ylabel("Efficiency")
    axes[2].grid(True, alpha=0.3)

    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def plot_amdahl(input_path: Path, output_path: Path, plt) -> None:
    threads, _, _, speedup, _, amdahl_speedup = load_scaling_dat(input_path)

    fig, ax = plt.subplots(figsize=(7, 5), constrained_layout=True)
    ax.plot(threads, speedup, "o-", linewidth=2, label="Measured speedup")
    ax.plot(threads, amdahl_speedup, "s--", linewidth=2, label="Amdahl speedup")
    ax.set_title("Measured vs Amdahl")
    ax.set_xlabel("Threads")
    ax.set_ylabel("Speedup")
    ax.grid(True, alpha=0.3)
    ax.legend()

    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot N-body benchmark outputs.")
    parser.add_argument("--input", required=True, help="Scaling .dat file")
    parser.add_argument("--output", required=True, help="PNG output for scaling plot")
    parser.add_argument("--amdahl-output", required=True, help="PNG output for Amdahl plot")
    args = parser.parse_args()

    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError:
        print(
            "matplotlib is not installed. Install it with: python3 -m pip install --user matplotlib",
            file=__import__("sys").stderr,
        )
        return 2

    input_path = Path(args.input)
    output_path = Path(args.output)
    amdahl_output_path = Path(args.amdahl_output)

    plot_scaling(input_path, output_path, plt)
    plot_amdahl(input_path, amdahl_output_path, plt)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
