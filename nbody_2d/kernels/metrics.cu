#include "metrics.cuh"
#include "../CudaCheck.cuh"
#include <cmath>
#include <stdexcept>

namespace {

// ---------------------------------
//  KERNELS DE ENERGÍA CINÉTICA (K)
// ---------------------------------

__global__ void kineticEnergyAtomicKernel(
    const double* mass, const double* vx, const double* vy, size_t n, double* totalK) 
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        double v2 = vx[i] * vx[i] + vy[i] * vy[i];
        double k = 0.5 * mass[i] * v2;
        // Variante obligatoria 1: Operación atómica global
        atomicAdd(totalK, k);
    }
}

__global__ void kineticEnergyReductionKernel(
    const double* mass, const double* vx, const double* vy, size_t n, double* totalK) 
{
    extern __shared__ double sdata[];
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    size_t tid = threadIdx.x;
    
    // Carga inicial en memoria compartida
    sdata[tid] = 0.0;
    if (i < n) {
        double v2 = vx[i] * vx[i] + vy[i] * vy[i];
        sdata[tid] = 0.5 * mass[i] * v2;
    }
    __syncthreads();
    
    // Patrón de reducción paralela estándar (requiere blockDim.x potencia de 2)
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }
    
    // El hilo maestro del bloque suma el resultado parcial al global
    if (tid == 0) {
        atomicAdd(totalK, sdata[0]);
    }
}

// ---------------------------------
// KERNELS DE ENERGÍA POTENCIAL (U)
// ---------------------------------

__global__ void potentialEnergyAtomicKernel(
    const double* mass, const double* x, const double* y, size_t n, 
    double G, double eps2, double* totalU) 
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    
    double u_i = 0.0;
    // Iterar solo j > i evita calcular pares duplicados y auto-interacciones
    for (size_t j = i + 1; j < n; ++j) {
        double dx = x[j] - x[i];
        double dy = y[j] - y[i];
        double dist = sqrt(dx*dx + dy*dy + eps2);
        u_i += -G * mass[i] * mass[j] / dist;
    }
    atomicAdd(totalU, u_i);
}

__global__ void potentialEnergyReductionKernel(
    const double* mass, const double* x, const double* y, size_t n, 
    double G, double eps2, double* totalU) 
{
    extern __shared__ double sdata[];
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    size_t tid = threadIdx.x;
    
    sdata[tid] = 0.0;
    if (i < n) {
        double u_i = 0.0;
        for (size_t j = i + 1; j < n; ++j) {
            double dx = x[j] - x[i];
            double dy = y[j] - y[i];
            double dist = sqrt(dx*dx + dy*dy + eps2);
            u_i += -G * mass[i] * mass[j] / dist;
        }
        sdata[tid] = u_i;
    }
    __syncthreads();
    
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }
    
    if (tid == 0) {
        atomicAdd(totalU, sdata[0]);
    }
}

}

// ---------------------------------------
// FUNCIONES LANZADORAS (Wrappers de C++)
// ---------------------------------------

double launchComputeKineticEnergy(
    const double* dMass, const double* dVx, const double* dVy, 
    std::size_t n, int method, int blockSize) 
{
    if (n == 0) return 0.0;

    double* dTotalK = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&dTotalK, sizeof(double)));
    CUDA_CHECK(cudaMemset(dTotalK, 0, sizeof(double))); // Inicializar en cero

    std::size_t gridSize = (n + blockSize - 1) / blockSize;

    if (method == 1) { // 1 = atomicAdd
        kineticEnergyAtomicKernel<<<gridSize, blockSize>>>(dMass, dVx, dVy, n, dTotalK);
    } else if (method == 0) { // 0 = reducción
        std::size_t sharedMemBytes = blockSize * sizeof(double);
        kineticEnergyReductionKernel<<<gridSize, blockSize, sharedMemBytes>>>(dMass, dVx, dVy, n, dTotalK);
    } else {
        cudaFree(dTotalK);
        throw std::invalid_argument("Metodo de energia cinetica invalido (debe ser 0 o 1).");
    }

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    double hostTotalK = 0.0;
    CUDA_CHECK(cudaMemcpy(&hostTotalK, dTotalK, sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(dTotalK));

    return hostTotalK;
}

double launchComputePotentialEnergy(
    const double* dMass, const double* dX, const double* dY, 
    std::size_t n, double G, double epsilon, int method, int blockSize) 
{
    if (n == 0) return 0.0;

    double* dTotalU = nullptr;
    CUDA_CHECK(cudaMalloc((void**)&dTotalU, sizeof(double)));
    CUDA_CHECK(cudaMemset(dTotalU, 0, sizeof(double)));

    double eps2 = epsilon * epsilon;
    std::size_t gridSize = (n + blockSize - 1) / blockSize;

    if (method == 1) { // 1 = atomicAdd
        potentialEnergyAtomicKernel<<<gridSize, blockSize>>>(dMass, dX, dY, n, G, eps2, dTotalU);
    } else if (method == 0) { // 0 = reducción
        std::size_t sharedMemBytes = blockSize * sizeof(double);
        potentialEnergyReductionKernel<<<gridSize, blockSize, sharedMemBytes>>>(dMass, dX, dY, n, G, eps2, dTotalU);
    } else {
        cudaFree(dTotalU);
        throw std::invalid_argument("Metodo de energia potencial invalido (debe ser 0 o 1).");
    }

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    double hostTotalU = 0.0;
    CUDA_CHECK(cudaMemcpy(&hostTotalU, dTotalU, sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(dTotalU));

    return hostTotalU;
}