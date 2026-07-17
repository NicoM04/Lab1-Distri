#pragma once

#include <cstddef>


/**
 * Contrato de integracion con la capa de memoria del Rol 2:
 *
 * - d_mass, d_x y d_y deben apuntar a arreglos de al menos
 *   n elementos double en memoria device.
 * - d_ax y d_ay deben apuntar a arreglos de al menos
 *   n elementos double en memoria device.
 * - Los lanzadores no reservan ni liberan memoria.
 * - Los lanzadores no realizan copias H2D o D2H.
 * - Los lanzadores no llaman cudaDeviceSynchronize().
 * - La capa que invoca el kernel controla la vida de los
 *   buffers, las transferencias y la sincronizacion.
 * - variant 0 selecciona el kernel basico.
 * - variant 1 selecciona el kernel shared.
 */




/**
 * Lanza el kernel basico de aceleraciones.
 *
 * Cada hilo CUDA calcula la aceleracion de un cuerpo i
 * recorriendo todos los cuerpos j en memoria global.
 */
void launchComputeAccelerationsBasic(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    std::size_t n,
    double gravitationalConstant,
    double epsilon,
    int blockSize
);

/**
 * Lanza el kernel de aceleraciones con memoria compartida.
 *
 * Las masas y posiciones se procesan mediante tiles
 * almacenados temporalmente en shared memory.
 */
void launchComputeAccelerationsShared(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    std::size_t n,
    double gravitationalConstant,
    double epsilon,
    int blockSize
);

/**
 * Selecciona la variante del kernel.
 *
 * variant = 0: kernel basico.
 * variant = 1: kernel con memoria compartida.
 */
void launchComputeAccelerations(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    std::size_t n,
    double gravitationalConstant,
    double epsilon,
    int variant,
    int blockSize
);