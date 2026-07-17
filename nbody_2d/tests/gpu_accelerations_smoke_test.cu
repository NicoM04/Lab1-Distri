#include "../CudaCheck.cuh"
#include "../kernels/accelerations.cuh"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

constexpr double gravitationalConstant = 1.0;
constexpr double epsilon = 0.1;
constexpr double relativeTolerance = 1e-4;
constexpr double absoluteTolerance = 1e-8;

bool approximatelyEqual(double actual, double expected) {
    const double difference = std::abs(actual - expected);

    return difference
        <= absoluteTolerance
        + relativeTolerance * std::abs(expected);
}

void computeAccelerationsCpu(
    const std::vector<double>& mass,
    const std::vector<double>& x,
    const std::vector<double>& y,
    std::vector<double>& ax,
    std::vector<double>& ay
) {
    const std::size_t n = mass.size();
    const double epsilonSquared = epsilon * epsilon;

    for (std::size_t i = 0; i < n; ++i) {
        double accelerationX = 0.0;
        double accelerationY = 0.0;

        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) {
                continue;
            }

            const double dx = x[j] - x[i];
            const double dy = y[j] - y[i];

            const double distanceSquared =
                dx * dx
                + dy * dy
                + epsilonSquared;

            const double inverseDistance =
                1.0 / std::sqrt(distanceSquared);

            const double inverseDistanceCubed =
                inverseDistance
                * inverseDistance
                * inverseDistance;

            const double factor =
                gravitationalConstant
                * mass[j]
                * inverseDistanceCubed;

            accelerationX += factor * dx;
            accelerationY += factor * dy;
        }

        ax[i] = accelerationX;
        ay[i] = accelerationY;
    }
}

bool compareResults(
    const std::vector<double>& expectedAx,
    const std::vector<double>& expectedAy,
    const std::vector<double>& actualAx,
    const std::vector<double>& actualAy,
    std::size_t n,
    int blockSize,
    const char* comparisonName
) {
    for (std::size_t i = 0; i < n; ++i) {
        const bool accelerationXCorrect =
            approximatelyEqual(actualAx[i], expectedAx[i]);

        const bool accelerationYCorrect =
            approximatelyEqual(actualAy[i], expectedAy[i]);

        if (!accelerationXCorrect || !accelerationYCorrect) {
            std::cerr
                << "Fallo en " << comparisonName << '\n'
                << "N = " << n
                << ", blockSize = " << blockSize
                << ", cuerpo i = " << i << '\n'
                << "Esperado: ax = " << expectedAx[i]
                << ", ay = " << expectedAy[i] << '\n'
                << "Obtenido: ax = " << actualAx[i]
                << ", ay = " << actualAy[i] << '\n';

            return false;
        }
    }

    return true;
}

bool runTestCase(std::size_t n, int blockSize) {
    std::vector<double> mass(n);
    std::vector<double> x(n);
    std::vector<double> y(n);

    for (std::size_t i = 0; i < n; ++i) {
        mass[i] = 1.0 + 0.05 * static_cast<double>(i);
        x[i] = 0.25 * static_cast<double>(i);
        y[i] = 0.10 * static_cast<double>(i % 5);
    }

    std::vector<double> cpuAx(n, 0.0);
    std::vector<double> cpuAy(n, 0.0);

    std::vector<double> basicAx(n, 0.0);
    std::vector<double> basicAy(n, 0.0);

    std::vector<double> sharedAx(n, 0.0);
    std::vector<double> sharedAy(n, 0.0);

    computeAccelerationsCpu(
        mass,
        x,
        y,
        cpuAx,
        cpuAy
    );

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
        mass.data(),
        bytes,
        cudaMemcpyHostToDevice
    ));

    CUDA_CHECK(cudaMemcpy(
        dX,
        x.data(),
        bytes,
        cudaMemcpyHostToDevice
    ));

    CUDA_CHECK(cudaMemcpy(
        dY,
        y.data(),
        bytes,
        cudaMemcpyHostToDevice
    ));

    // Variante 0: kernel basico.
    launchComputeAccelerations(
        dMass,
        dX,
        dY,
        dAx,
        dAy,
        n,
        gravitationalConstant,
        epsilon,
        0,
        blockSize
    );

    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(
        basicAx.data(),
        dAx,
        bytes,
        cudaMemcpyDeviceToHost
    ));

    CUDA_CHECK(cudaMemcpy(
        basicAy.data(),
        dAy,
        bytes,
        cudaMemcpyDeviceToHost
    ));

    // Variante 1: kernel con shared memory.
    launchComputeAccelerations(
        dMass,
        dX,
        dY,
        dAx,
        dAy,
        n,
        gravitationalConstant,
        epsilon,
        1,
        blockSize
    );

    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(
        sharedAx.data(),
        dAx,
        bytes,
        cudaMemcpyDeviceToHost
    ));

    CUDA_CHECK(cudaMemcpy(
        sharedAy.data(),
        dAy,
        bytes,
        cudaMemcpyDeviceToHost
    ));

    CUDA_CHECK(cudaFree(dMass));
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    CUDA_CHECK(cudaFree(dAx));
    CUDA_CHECK(cudaFree(dAy));

    const bool basicMatchesCpu =
        compareResults(
            cpuAx,
            cpuAy,
            basicAx,
            basicAy,
            n,
            blockSize,
            "CPU vs kernel basico"
        );

    const bool sharedMatchesCpu =
        compareResults(
            cpuAx,
            cpuAy,
            sharedAx,
            sharedAy,
            n,
            blockSize,
            "CPU vs kernel shared"
        );

    const bool sharedMatchesBasic =
        compareResults(
            basicAx,
            basicAy,
            sharedAx,
            sharedAy,
            n,
            blockSize,
            "kernel basico vs kernel shared"
        );

    if (
        !basicMatchesCpu
        || !sharedMatchesCpu
        || !sharedMatchesBasic
    ) {
        return false;
    }

    std::cout
        << "PASS: N = " << n
        << ", blockSize = " << blockSize
        << ", variantes basica y shared\n";

    return true;
}

bool testInvalidVariant() {
    try {
        launchComputeAccelerations(
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            1,
            gravitationalConstant,
            epsilon,
            2,
            64
        );
    } catch (const std::invalid_argument&) {
        std::cout
            << "PASS: variante CUDA invalida detectada\n";

        return true;
    }

    std::cerr
        << "FAIL: no se detecto la variante CUDA invalida\n";

    return false;
}

} // namespace

int main() {
    bool allTestsPassed = true;

    allTestsPassed &= runTestCase(2, 64);
    allTestsPassed &= runTestCase(3, 64);
    allTestsPassed &= runTestCase(31, 16);
    allTestsPassed &= runTestCase(32, 16);
    allTestsPassed &= runTestCase(33, 16);

    allTestsPassed &= testInvalidVariant();

    if (!allTestsPassed) {
        std::cerr
            << "Kernels CUDA de aceleraciones: FAIL\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "Kernels CUDA basico y shared: "
        << "TODOS LOS CASOS PASS\n";

    return EXIT_SUCCESS;
}