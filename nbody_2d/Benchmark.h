#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <cstddef>

#include "NBodySystem.h"

struct BenchmarkComparisonResult {
    double serial_seconds;
    double parallel_seconds;
    double speedup;
    double max_abs_acc_diff;
};

class Benchmark {
public:
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