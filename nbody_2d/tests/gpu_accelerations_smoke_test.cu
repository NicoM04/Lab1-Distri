#include "../CudaCheck.cuh"
#include "../kernels/accelerations.cuh"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

bool approximatelyEqual(
    double actual,
    double expected,
    double relativeTolerance = 1e-4,
    double absoluteTolerance = 1e-8
) {
    const double difference = std::abs(actual - expected);

    return difference
        <= absoluteTolerance
        + relativeTolerance * std::abs(expected);
}

} // namespace

int main() {
    constexpr std::size_t n = 2;

    const double hMass[n] = {1.0, 1.0};
    const double hX[n] = {0.0, 1.0};
    const double hY[n] = {0.0, 0.0};

    double hAx[n] = {0.0, 0.0};
    double hAy[n] = {0.0, 0.0};

    double* dMass = nullptr;
    double* dX = nullptr;
    double* dY = nullptr;
    double* dAx = nullptr;
    double* dAy = nullptr;

    const std::size_t bytes = n * sizeof(double);

    CUDA_CHECK(cudaMalloc(&dMass, bytes));
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    CUDA_CHECK(cudaMalloc(&dY, bytes));
    CUDA_CHECK(cudaMalloc(&dAx, bytes));
    CUDA_CHECK(cudaMalloc(&dAy, bytes));

    CUDA_CHECK(cudaMemcpy(
        dMass,
        hMass,
        bytes,
        cudaMemcpyHostToDevice
    ));

    CUDA_CHECK(cudaMemcpy(
        dX,
        hX,
        bytes,
        cudaMemcpyHostToDevice
    ));

    CUDA_CHECK(cudaMemcpy(
        dY,
        hY,
        bytes,
        cudaMemcpyHostToDevice
    ));

    launchComputeAccelerationsBasic(
        dMass,
        dX,
        dY,
        dAx,
        dAy,
        n,
        1.0,
        0.1,
        64
    );

    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(
        hAx,
        dAx,
        bytes,
        cudaMemcpyDeviceToHost
    ));

    CUDA_CHECK(cudaMemcpy(
        hAy,
        dAy,
        bytes,
        cudaMemcpyDeviceToHost
    ));

    CUDA_CHECK(cudaFree(dMass));
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    CUDA_CHECK(cudaFree(dAx));
    CUDA_CHECK(cudaFree(dAy));

    const double expectedAcceleration =
        1.0 / std::pow(1.0 + 0.1 * 0.1, 1.5);

    const bool firstBodyCorrect =
        approximatelyEqual(hAx[0], expectedAcceleration)
        && approximatelyEqual(hAy[0], 0.0);

    const bool secondBodyCorrect =
        approximatelyEqual(hAx[1], -expectedAcceleration)
        && approximatelyEqual(hAy[1], 0.0);

    if (!firstBodyCorrect || !secondBodyCorrect) {
        std::cerr
            << "Prueba GPU fallida\n"
            << "Cuerpo 0: ax = " << hAx[0]
            << ", ay = " << hAy[0] << '\n'
            << "Cuerpo 1: ax = " << hAx[1]
            << ", ay = " << hAy[1] << '\n'
            << "Esperado: +/-"
            << expectedAcceleration
            << '\n';

        return EXIT_FAILURE;
    }

    std::cout
        << "Kernel CUDA basico: PASS\n"
        << "Cuerpo 0: ax = " << hAx[0]
        << ", ay = " << hAy[0] << '\n'
        << "Cuerpo 1: ax = " << hAx[1]
        << ", ay = " << hAy[1] << '\n';

    return EXIT_SUCCESS;
}