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

    // Matrix N
    std::vector<std::size_t> N_values = {256, 512, 1024, 2000};
    const int default_blockdim = 256;
    
    std::ofstream bench_out(output_dir + "/benchmark_results.dat");
    bench_out << "N\tCPU_Time(ms)\tGPU_Basic_Kernel(ms)\tGPU_Basic_E2E(ms)\tGPU_Shared_Kernel(ms)\tGPU_Shared_E2E(ms)\n";
    
    std::ofstream scale_out(output_dir + "/scaling_analysis.dat");
    scale_out << "N\tSpeedup_Basic\tSpeedup_Shared\tAmdahl_Pred_Shared\n";
    
    std::cout << "[CUDA Benchmark] N Scaling analysis...\n";
    for (std::size_t n : N_values) {
        std::cout << "  -> N=" << n << std::flush;
        double cpu = measureCpuTime(n, G, eps, repetitions, dt);
        double gpu_basic_k = measureGpuKernelOnly(n, G, eps, 0, default_blockdim, repetitions);
        double gpu_basic_e = measureGpuEndToEnd(n, G, eps, 0, default_blockdim, repetitions, dt);
        double gpu_shared_k = measureGpuKernelOnly(n, G, eps, 1, default_blockdim, repetitions);
        double gpu_shared_e = measureGpuEndToEnd(n, G, eps, 1, default_blockdim, repetitions, dt);
        
        bench_out << n << "\t" << cpu << "\t" 
                  << gpu_basic_k << "\t" << gpu_basic_e << "\t"
                  << gpu_shared_k << "\t" << gpu_shared_e << "\n";
                  
        double sp_basic = cpu / gpu_basic_e;
        double sp_shared = cpu / gpu_shared_e;
        
        // Empiric Serial Fraction for Amdahl
        // S = 1 / (f + (1-f)/Sp_k) where Sp_k is kernel speedup (cpu / gpu_shared_k)
        double sp_k = cpu / gpu_shared_k;
        double transfer_time = gpu_shared_e - gpu_shared_k;
        double f = transfer_time / (cpu + transfer_time);
        double amdahl = 1.0 / (f + (1.0 - f) / sp_k);
        
        scale_out << n << "\t" << sp_basic << "\t" << sp_shared << "\t" << amdahl << "\n";
        std::cout << " (Done)\n";
    }
    
    // BlockDim Study
    std::cout << "[CUDA Benchmark] blockDim.x study (N=2000)...\n";
    std::vector<int> blockdims = {64, 128, 256, 512, 1024};
    std::size_t n_block = 2000;
    
    std::ofstream bdim_out(output_dir + "/blockdim_study.dat");
    bdim_out << "BlockDim\tTime_Basic(ms)\tTime_Shared(ms)\n";
    
    for (int bdim : blockdims) {
        std::cout << "  -> blockDim=" << bdim << std::flush;
        double basic_k = measureGpuKernelOnly(n_block, G, eps, 0, bdim, repetitions);
        double shared_k = measureGpuKernelOnly(n_block, G, eps, 1, bdim, repetitions);
        bdim_out << bdim << "\t" << basic_k << "\t" << shared_k << "\n";
        std::cout << " (Done)\n";
    }
    
    std::cout << "[CUDA Benchmark] All .dat files generated in " << output_dir << "\n";
}

}
