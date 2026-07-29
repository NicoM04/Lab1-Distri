#pragma once

#include <cstddef>

/**
 * Lanza el kernel para calcular la energía cinética global en la GPU.
 * @param method 0 = Reducción en shared memory, 1 = atomicAdd
 */
double launchComputeKineticEnergy(
    const double* dMass,
    const double* dVx,
    const double* dVy,
    std::size_t n,
    int method,
    int blockSize
);

/**
 * Lanza el kernel para calcular la energía potencial global en la GPU.
 * @param method 0 = Reducción en shared memory, 1 = atomicAdd
 */
double launchComputePotentialEnergy(
    const double* dMass,
    const double* dX,
    const double* dY,
    std::size_t n,
    double G,
    double epsilon,
    int method,
    int blockSize
);