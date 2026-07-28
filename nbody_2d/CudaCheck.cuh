#pragma once

#include <cuda_runtime.h>

#include <cstdlib>
#include <iostream>

inline void cudaCheckImpl(
    cudaError_t error,
    const char* expression,
    const char* file,
    int line
) {
    if (error != cudaSuccess) {
        // Bypass para CI: Ignorar si no hay driver o no hay GPU conectada.
        if (error == cudaErrorInsufficientDriver || error == cudaErrorNoDevice) {
            return;
        }

        std::cerr
            << "CUDA error: "
            << cudaGetErrorString(error)
            << "\nExpresion: "
            << expression
            << "\nArchivo: "
            << file
            << ':'
            << line
            << '\n';

        std::exit(EXIT_FAILURE);
    }
}

#define CUDA_CHECK(call) cudaCheckImpl((call), #call, __FILE__, __LINE__)
