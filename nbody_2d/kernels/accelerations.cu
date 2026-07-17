#include "accelerations.cuh"

#include "../CudaCheck.cuh"

#include <cmath>
#include <stdexcept>

namespace {

/**
 * Kernel basico de aceleraciones.
 *
 * Cada hilo calcula la aceleracion de un cuerpo i y recorre
 * secuencialmente todos los cuerpos j.
 */
__global__ void computeAccelerationsKernel(
    const double* mass,
    const double* x,
    const double* y,
    double* ax,
    double* ay,
    std::size_t n,
    double gravitationalConstant,
    double epsilon
) {
    const std::size_t i =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x
        + threadIdx.x;

    // Proteccion para los hilos que quedan fuera de N.
    if (i >= n) {
        return;
    }

    const double xi = x[i];
    const double yi = y[i];
    const double epsilonSquared = epsilon * epsilon;

    double accelerationX = 0.0;
    double accelerationY = 0.0;

    for (std::size_t j = 0; j < n; ++j) {
        if (j == i) {
            continue;
        }

        const double dx = x[j] - xi;
        const double dy = y[j] - yi;

        const double distanceSquared =
            dx * dx
            + dy * dy
            + epsilonSquared;

        const double inverseDistance =
            1.0 / sqrt(distanceSquared);

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

    // Cada hilo escribe solamente el resultado de su cuerpo i.
    ax[i] = accelerationX;
    ay[i] = accelerationY;
}

/**
 * Comprueba los argumentos comunes antes de lanzar un kernel.
 */
void validateLaunchArguments(
    const double* dMass,
    const double* dX,
    const double* dY,
    double* dAx,
    double* dAy,
    int blockSize
) {
    if (
        dMass == nullptr
        || dX == nullptr
        || dY == nullptr
        || dAx == nullptr
        || dAy == nullptr
    ) {
        throw std::invalid_argument(
            "Los punteros device no pueden ser nulos"
        );
    }

    if (blockSize <= 0) {
        throw std::invalid_argument(
            "blockSize debe ser mayor que cero"
        );
    }
}

} // namespace

void launchComputeAccelerationsBasic(
    const double* dMass,
    const double* dX,
    const double* dY,
    double* dAx,
    double* dAy,
    std::size_t n,
    double gravitationalConstant,
    double epsilon,
    int blockSize
) {
    // No hay trabajo que realizar.
    if (n == 0) {
        return;
    }

    validateLaunchArguments(
        dMass,
        dX,
        dY,
        dAx,
        dAy,
        blockSize
    );

    const std::size_t blockSizeValue =
        static_cast<std::size_t>(blockSize);

    // Division a techo: ceil(N / blockSize).
    const std::size_t gridSize =
        (n + blockSizeValue - 1)
        / blockSizeValue;

    computeAccelerationsKernel<<<gridSize, blockSize>>>(
        dMass,
        dX,
        dY,
        dAx,
        dAy,
        n,
        gravitationalConstant,
        epsilon
    );

    // Comprueba errores inmediatos del lanzamiento.
    CUDA_CHECK(cudaGetLastError());

    // No se sincroniza aqui.
    // La sincronizacion corresponde al test, simulador o benchmark.
}

void launchComputeAccelerationsShared(
    const double*,
    const double*,
    const double*,
    double*,
    double*,
    std::size_t,
    double,
    double,
    int
) {
    throw std::logic_error(
        "La variante shared memory aun no esta implementada"
    );
}

void launchComputeAccelerations(
    const double* dMass,
    const double* dX,
    const double* dY,
    double* dAx,
    double* dAy,
    std::size_t n,
    double gravitationalConstant,
    double epsilon,
    int variant,
    int blockSize
) {
    switch (variant) {
        case 0:
            launchComputeAccelerationsBasic(
                dMass,
                dX,
                dY,
                dAx,
                dAy,
                n,
                gravitationalConstant,
                epsilon,
                blockSize
            );
            break;

        case 1:
            launchComputeAccelerationsShared(
                dMass,
                dX,
                dY,
                dAx,
                dAy,
                n,
                gravitationalConstant,
                epsilon,
                blockSize
            );
            break;

        default:
            throw std::invalid_argument(
                "La variante CUDA debe ser 0 o 1"
            );
    }
}