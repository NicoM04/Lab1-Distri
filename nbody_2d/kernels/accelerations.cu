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
 * Kernel de aceleraciones con memoria compartida.
 *
 * Cada bloque procesa las masas y posiciones en tiles.
 * Todos los hilos del bloque participan en la carga del tile,
 * incluso aquellos cuyo indice i queda fuera de N.
 */
__global__ void computeAccelerationsKernelShared(
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

    /*
     * Layout de la memoria compartida:
     *
     * sharedMass[blockDim.x]
     * sharedX[blockDim.x]
     * sharedY[blockDim.x]
     */
    extern __shared__ double sharedData[];

    double* sharedMass = sharedData;
    double* sharedX = sharedMass + blockDim.x;
    double* sharedY = sharedX + blockDim.x;

    const bool active = i < n;

    double xi = 0.0;
    double yi = 0.0;

    if (active) {
        xi = x[i];
        yi = y[i];
    }

    const double epsilonSquared = epsilon * epsilon;

    double accelerationX = 0.0;
    double accelerationY = 0.0;

    for (
        std::size_t tileStart = 0;
        tileStart < n;
        tileStart += blockDim.x
    ) {
        const std::size_t sourceIndex =
            tileStart + threadIdx.x;

        /*
         * Cada hilo carga como maximo un cuerpo.
         * En el ultimo tile puede haber indices fuera de N.
         */
        if (sourceIndex < n) {
            sharedMass[threadIdx.x] = mass[sourceIndex];
            sharedX[threadIdx.x] = x[sourceIndex];
            sharedY[threadIdx.x] = y[sourceIndex];
        } else {
            sharedMass[threadIdx.x] = 0.0;
            sharedX[threadIdx.x] = 0.0;
            sharedY[threadIdx.x] = 0.0;
        }

        /*
         * Ningun hilo puede comenzar a usar el tile hasta que
         * todos hayan terminado de cargarlo.
         */
        __syncthreads();

        const std::size_t remainingBodies =
            n - tileStart;

        const std::size_t tileSize =
            remainingBodies
                < static_cast<std::size_t>(blockDim.x)
            ? remainingBodies
            : static_cast<std::size_t>(blockDim.x);

        if (active) {
            for (
                std::size_t tileIndex = 0;
                tileIndex < tileSize;
                ++tileIndex
            ) {
                const std::size_t j =
                    tileStart + tileIndex;

                if (j == i) {
                    continue;
                }

                const double dx =
                    sharedX[tileIndex] - xi;

                const double dy =
                    sharedY[tileIndex] - yi;

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
                    * sharedMass[tileIndex]
                    * inverseDistanceCubed;

                accelerationX += factor * dx;
                accelerationY += factor * dy;
            }
        }

        /*
         * Evita que algunos hilos sobrescriban el tile mientras
         * otros aun lo estan utilizando.
         */
        __syncthreads();
    }

    if (active) {
        ax[i] = accelerationX;
        ay[i] = accelerationY;
    }
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

    const std::size_t gridSize =
        (n + blockSizeValue - 1)
        / blockSizeValue;

    /*
     * Se almacenan tres arreglos de double por bloque:
     * masas, posiciones X y posiciones Y.
     */
    const std::size_t sharedMemoryBytes =
        3
        * blockSizeValue
        * sizeof(double);

    computeAccelerationsKernelShared
        <<<gridSize, blockSize, sharedMemoryBytes>>>(
            dMass,
            dX,
            dY,
            dAx,
            dAy,
            n,
            gravitationalConstant,
            epsilon
        );

    CUDA_CHECK(cudaGetLastError());

    // La sincronizacion se realiza fuera del lanzador.
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