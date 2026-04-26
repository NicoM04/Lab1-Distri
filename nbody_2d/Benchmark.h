#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <cstddef>
#include <string>
#include <vector>

#include "NBodySystem.h"

struct BenchmarkStats {
    double mean;
    double stddev;
};

struct BenchmarkRunResult {
    int threads;
    std::size_t problem_size;
    int schedule_type;
    int chunk_size;
    int repetitions;
    BenchmarkStats compute_time;
    BenchmarkStats full_step_time;

    double compute_speedup;
    double compute_efficiency;
    double compute_speedup_std;
    double compute_efficiency_std;

    double full_step_speedup;
    double full_step_efficiency;
    double full_step_speedup_std;
    double full_step_efficiency_std;
};

struct AmdahlResult {
    int threads;
    double measured_speedup;
    double amdahl_speedup;
    double serial_fraction;
};

struct BenchmarkComparisonResult {
    double serial_seconds;
    double parallel_seconds;
    double speedup;
    double max_abs_acc_diff;
};

class Benchmark {
public:
    static double measureComputeAccelerationsTime(NBodySystem& system, int schedule_type, int chunk_size);
    static double measureFullStepTime(NBodySystem& system, double dt, int schedule_type, int chunk_size);

    static BenchmarkRunResult runExperiment(
        std::size_t n,
        int threads,
        int repetitions,
        double dt,
        double G,
        double eps,
        int schedule_type,
        int chunk_size,
        unsigned int seed
    );

    static std::vector<BenchmarkRunResult> runScalingAnalysis(
        std::size_t n,
        const std::vector<int>& thread_counts,
        int repetitions,
        double dt,
        double G,
        double eps,
        int schedule_type,
        int chunk_size,
        unsigned int seed
    );

    static std::vector<BenchmarkRunResult> runScheduleComparison(
        std::size_t n,
        int threads,
        int repetitions,
        double dt,
        double G,
        double eps,
        const std::vector<int>& schedule_types,
        const std::vector<int>& chunk_sizes,
        unsigned int seed
    );

    static bool exportScalingDat(
        const std::string& output_path,
        const std::vector<BenchmarkRunResult>& results,
        bool use_full_step_time
    );

    static bool exportScheduleDat(
        const std::string& output_path,
        const std::vector<BenchmarkRunResult>& results,
        bool use_full_step_time
    );

    static std::vector<AmdahlResult> runAmdahlAnalysis(
        const std::vector<BenchmarkRunResult>& results,
        bool use_full_step_time
    );

    static bool exportAmdahlDat(
        const std::string& output_path,
        const std::vector<AmdahlResult>& results
    );

    static const char* scheduleName(int schedule_type);

    // Backward-compatible helpers used by previous weeks.
    static double runSteps(NBodySystem& system, int steps, double dt);
    static BenchmarkComparisonResult compareSerialVsParallel(
        std::size_t n,
        int steps,
        double dt,
        double G,
        double eps,
        int schedule_type,
        int chunk_size,
        unsigned int seed
    );
};

#endif