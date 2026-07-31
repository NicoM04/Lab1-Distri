#include "BenchmarkCuda.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>

#include "NBodySystem.h"
#include "Integrator.h"

namespace BenchmarkCuda {

double wallTimeSeconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

double measureCpuTime(std::size_t n, double G, double eps, int repetitions, double dt) {
    double total_time = 0.0;
    for (int r = 0; r < repetitions; ++r) {
        NBodySystem sys(G, eps);
        // Add dummy particles
        for (std::size_t i = 0; i < n; ++i) {
            sys.addParticle(Particle(1.0, i*0.1, i*0.1));
        }
        
        const double t0 = wallTimeSeconds();
        sys.computeAccelerationsSerial();
        Integrator::eulerStep(sys, dt, -1);
        const double t1 = wallTimeSeconds();
        total_time += (t1 - t0);
    }
    return (total_time / repetitions) * 1000.0; // in ms
}

double measureGpuKernelOnly(std::size_t n, double G, double eps, int variant, int blockDim, int repetitions) {
    NBodySystem sys(G, eps);
    for (std::size_t i = 0; i < n; ++i) {
        sys.addParticle(Particle(1.0, i*0.1, i*0.1));
    }
    // Allocate memory on device
    sys.allocateDeviceMemory(n);
    sys.uploadStateToDevice();
    
    // Warmup
    sys.computeAccelerationsGpuKernelOnly(variant, blockDim);
    
    double total_time = 0.0;
    for (int r = 0; r < repetitions; ++r) {
        const double t0 = wallTimeSeconds();
        sys.computeAccelerationsGpuKernelOnly(variant, blockDim);
        const double t1 = wallTimeSeconds();
        total_time += (t1 - t0);
    }
    return (total_time / repetitions) * 1000.0; // in ms
}

double measureGpuEndToEnd(std::size_t n, double G, double eps, int variant, int blockDim, int repetitions, double dt) {
    double total_time = 0.0;
    for (int r = 0; r < repetitions; ++r) {
        NBodySystem sys(G, eps);
        for (std::size_t i = 0; i < n; ++i) {
            sys.addParticle(Particle(1.0, i*0.1, i*0.1));
        }
        
        // Warmup allocator
        sys.allocateDeviceMemory(n);
        
        const double t0 = wallTimeSeconds();
        sys.computeAccelerationsGpu(variant, blockDim); // End to end step
        Integrator::eulerStep(sys, dt, -1);
        const double t1 = wallTimeSeconds();
        total_time += (t1 - t0);
    }
    return (total_time / repetitions) * 1000.0; // in ms
}

void runCudaBenchmarks(int repetitions, double dt, unsigned int seed, const std::string& output_dir) {
    const double G = 1.0;
    const double eps = 0.1;
    
    std::cout << "\n[CUDA Benchmark] Starting benchmarking suite...\n";

    std::vector<std::size_t> N_values = {256, 512, 1024, 2000};
    std::vector<int> blockdims = {64, 128, 256, 512, 1024};
    
    std::ofstream bench_out(output_dir + "/benchmark_results.dat");
    bench_out << "N\tBlockDim\tCPU_Time(ms)\tGPU_Basic_Kernel(ms)\tGPU_Basic_E2E(ms)\tGPU_Shared_Kernel(ms)\tGPU_Shared_E2E(ms)\n";
    
    std::cout << "[CUDA Benchmark] Matrix Evaluation (N x blockDim.x)...\n";
    for (std::size_t n : N_values) {
        double cpu = measureCpuTime(n, G, eps, repetitions, dt);
        for (int bdim : blockdims) {
            std::cout << "  -> N=" << n << " blockDim=" << bdim << std::flush;
            double gpu_basic_k = measureGpuKernelOnly(n, G, eps, 0, bdim, repetitions);
            double gpu_basic_e = measureGpuEndToEnd(n, G, eps, 0, bdim, repetitions, dt);
            double gpu_shared_k = measureGpuKernelOnly(n, G, eps, 1, bdim, repetitions);
            double gpu_shared_e = measureGpuEndToEnd(n, G, eps, 1, bdim, repetitions, dt);
            
            bench_out << n << "\t" << bdim << "\t" << cpu << "\t" 
                      << gpu_basic_k << "\t" << gpu_basic_e << "\t"
                      << gpu_shared_k << "\t" << gpu_shared_e << "\n";
            std::cout << " (Done)\n";
        }
    }
    
    std::cout << "[CUDA Benchmark] All .dat files generated in " << output_dir << "\n";
}

}
