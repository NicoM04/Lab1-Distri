#!/usr/bin/env python3
"""Generate performance and physics reports from N-body .dat outputs."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List, Tuple


def _load_rows(path: Path) -> List[List[str]]:
    rows: List[List[str]] = []
    with path.open("r", encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            rows.append(line.split())
    return rows


def load_scaling(path: Path) -> Tuple[List[int], List[float], List[float], List[float]]:
    rows = _load_rows(path)
    threads: List[int] = []
    speedup: List[float] = []
    efficiency: List[float] = []
    amdahl: List[float] = []

    for parts in rows:
        if len(parts) < 6:
            continue
        threads.append(int(parts[0]))
        speedup.append(float(parts[3]))
        efficiency.append(float(parts[4]))
        amdahl.append(float(parts[5]))

    return threads, speedup, efficiency, amdahl


def load_schedules(path: Path) -> Dict[str, List[Tuple[int, float]]]:
    rows = _load_rows(path)
    grouped: Dict[str, List[Tuple[int, float]]] = {}

    for parts in rows:
        if len(parts) < 5:
            continue
        schedule = parts[0]
        chunk = int(parts[1])
        time_mean = float(parts[3])
        grouped.setdefault(schedule, []).append((chunk, time_mean))

    for schedule in grouped:
        grouped[schedule].sort(key=lambda item: item[0])

    return grouped


def load_trajectories(path: Path) -> Dict[int, Tuple[List[float], List[float]]]:
    rows = _load_rows(path)
    series: Dict[int, Tuple[List[float], List[float]]] = {}

    for parts in rows:
        if len(parts) < 5:
            continue
        body_id = int(parts[2])
        x = float(parts[3])
        y = float(parts[4])
        if body_id not in series:
            series[body_id] = ([], [])
        series[body_id][0].append(x)
        series[body_id][1].append(y)

    return series


def load_energy_drift(path: Path) -> Tuple[List[float], List[float]]:
    rows = _load_rows(path)
    times: List[float] = []
    drift: List[float] = []

    total_energy: List[float] = []
    for parts in rows:
        if len(parts) < 5:
            continue
        times.append(float(parts[1]))
        total_energy.append(float(parts[4]))

    if not total_energy:
        return times, drift

    e0 = total_energy[0]
    denom = abs(e0) if abs(e0) > 1e-18 else 1.0
    for e in total_energy:
        drift.append((e - e0) / denom)

    return times, drift


def plot_performance(scaling_path: Path, schedules_path: Path, output_path: Path, plt) -> None:
    threads, speedup, efficiency, amdahl = load_scaling(scaling_path)
    schedules = load_schedules(schedules_path)

    fig, axes = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)
    fig.suptitle("N-Body Performance Analysis", fontsize=16)

    ax_speedup = axes[0][0]
    ax_speedup.plot(threads, speedup, "o-", linewidth=2, label="Measured speedup")
    ax_speedup.plot(threads, amdahl, "s--", linewidth=2, label="Amdahl curve")
    ax_speedup.set_title("Speedup vs Threads")
    ax_speedup.set_xlabel("Threads")
    ax_speedup.set_ylabel("Speedup")
    ax_speedup.grid(True, alpha=0.3)
    ax_speedup.legend()

    ax_eff = axes[0][1]
    ax_eff.plot(threads, efficiency, "o-", linewidth=2, color="#ff7f0e")
    ax_eff.set_title("Efficiency vs Threads")
    ax_eff.set_xlabel("Threads")
    ax_eff.set_ylabel("Efficiency")
    ax_eff.grid(True, alpha=0.3)

    ax_sched = axes[1][0]
    for schedule_name, values in schedules.items():
        chunks = [item[0] for item in values]
        mean_times = [item[1] for item in values]
        ax_sched.plot(chunks, mean_times, "o-", linewidth=2, label=schedule_name)
    ax_sched.set_title("Schedule Comparison by Chunk")
    ax_sched.set_xlabel("Chunk size")
    ax_sched.set_ylabel("Mean time [s]")
    ax_sched.grid(True, alpha=0.3)
    ax_sched.legend()

    ax_amdahl = axes[1][1]
    ax_amdahl.plot(threads, speedup, "o-", linewidth=2, label="Measured")
    ax_amdahl.plot(threads, amdahl, "s--", linewidth=2, label="Amdahl")
    ax_amdahl.fill_between(threads, amdahl, speedup, alpha=0.15, color="#2ca02c")
    ax_amdahl.set_title("Amdahl Fit")
    ax_amdahl.set_xlabel("Threads")
    ax_amdahl.set_ylabel("Speedup")
    ax_amdahl.grid(True, alpha=0.3)
    ax_amdahl.legend()

    fig.savefig(output_path, dpi=220, bbox_inches="tight")
    plt.close(fig)


def plot_physics(trajectories_path: Path, energy_path: Path, output_path: Path, subset_bodies: int, plt) -> None:
    trajectories = load_trajectories(trajectories_path)
    times, drift = load_energy_drift(energy_path)

    fig, axes = plt.subplots(1, 2, figsize=(14, 5), constrained_layout=True)
    fig.suptitle("N-Body Physics Diagnostics", fontsize=16)

    ax_traj = axes[0]
    selected_ids = sorted(trajectories.keys())[: max(1, subset_bodies)]
    for body_id in selected_ids:
        xs, ys = trajectories[body_id]
        ax_traj.plot(xs, ys, linewidth=1.6, label=f"body {body_id}")
    ax_traj.set_title("Trajectories (subset)")
    ax_traj.set_xlabel("x")
    ax_traj.set_ylabel("y")
    ax_traj.grid(True, alpha=0.3)
    if len(selected_ids) <= 12:
        ax_traj.legend(ncol=2, fontsize=8)

    ax_drift = axes[1]
    ax_drift.plot(times, drift, color="#d62728", linewidth=2)
    ax_drift.axhline(0.0, color="black", linewidth=1, alpha=0.7)
    ax_drift.set_title("Total Energy Drift")
    ax_drift.set_xlabel("time")
    ax_drift.set_ylabel("(E(t)-E0)/|E0|")
    ax_drift.grid(True, alpha=0.3)

    fig.savefig(output_path, dpi=220, bbox_inches="tight")
    plt.close(fig)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate PNG reports from N-body dat files.")
    parser.add_argument("--scaling", help="Path to benchmark scaling dat")
    parser.add_argument("--schedules", help="Path to benchmark schedules dat")
    parser.add_argument("--trajectories", help="Path to trajectories dat")
    parser.add_argument("--energy", help="Path to energy timeseries dat")
    parser.add_argument("--performance-output", help="Output PNG for performance report")
    parser.add_argument("--physics-output", help="Output PNG for physics report")
    parser.add_argument("--subset-bodies", type=int, default=10, help="How many bodies to draw in trajectories")
    args = parser.parse_args()

    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError:
        print(
            "matplotlib is not installed. Install it with: python3 -m pip install --user matplotlib",
            file=__import__("sys").stderr,
        )
        return 2

    do_performance = bool(args.performance_output)
    do_physics = bool(args.physics_output)

    if not do_performance and not do_physics:
        print(
            "Nothing to do. Provide --performance-output and/or --physics-output.",
            file=__import__("sys").stderr,
        )
        return 1

    if do_performance:
        if not args.scaling or not args.schedules:
            print(
                "For performance plot you must provide --scaling and --schedules.",
                file=__import__("sys").stderr,
            )
            return 1
        scaling_path = Path(args.scaling)
        schedules_path = Path(args.schedules)
        missing_performance = [str(path) for path in (scaling_path, schedules_path) if not path.exists()]
        if missing_performance:
            print("Missing performance input files:", file=__import__("sys").stderr)
            for path in missing_performance:
                print(f"  - {path}", file=__import__("sys").stderr)
            return 1
        plot_performance(scaling_path, schedules_path, Path(args.performance_output), plt)

    if do_physics:
        if not args.trajectories or not args.energy:
            print(
                "For physics plot you must provide --trajectories and --energy.",
                file=__import__("sys").stderr,
            )
            return 1
        trajectories_path = Path(args.trajectories)
        energy_path = Path(args.energy)
        missing_physics = [str(path) for path in (trajectories_path, energy_path) if not path.exists()]
        if missing_physics:
            print("Missing physics input files:", file=__import__("sys").stderr)
            for path in missing_physics:
                print(f"  - {path}", file=__import__("sys").stderr)
            return 1
        plot_physics(
            trajectories_path,
            energy_path,
            Path(args.physics_output),
            max(1, args.subset_bodies),
            plt,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
