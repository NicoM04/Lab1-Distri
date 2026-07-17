#pragma once

#include <cstddef>

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