#ifndef BENCHMARK_CUDA_H
#define BENCHMARK_CUDA_H

#include <string>

namespace BenchmarkCuda {
    // Runs the CUDA specific benchmarks (N, blockDim matrix) and writes .dat files
    void runCudaBenchmarks(int repetitions, double dt, unsigned int seed, const std::string& output_dir);
}

#endif // BENCHMARK_CUDA_H
