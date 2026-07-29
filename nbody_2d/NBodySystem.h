#ifndef NBODYSYSTEM_H
#define NBODYSYSTEM_H

#include <cstddef>
#include <vector>

#include "CudaBuffer.h"
#include "Particle.h"

class NBodySystem {
private:
    std::vector<Particle> bodies_;
    double G_const_;
    double softening_eps_;

    CudaBuffer<double> d_mass_;
    CudaBuffer<double> d_x_;
    CudaBuffer<double> d_y_;
    CudaBuffer<double> d_vx_;
    CudaBuffer<double> d_vy_;
    CudaBuffer<double> d_ax_;
    CudaBuffer<double> d_ay_;

    std::size_t device_body_count_;
    std::size_t device_transfer_count_;
    bool device_mass_uploaded_;
    bool host_state_dirty_;

    void ensureDeviceCapacity(std::size_t n_bodies);

public:
    NBodySystem(double G_const, double softening_eps);
    NBodySystem(const NBodySystem&) = delete;
    NBodySystem& operator=(const NBodySystem&) = delete;
    NBodySystem(NBodySystem&&) noexcept = default;
    NBodySystem& operator=(NBodySystem&&) noexcept = default;

    void addParticle(const Particle& particle);
    void zeroAccelerations();

    void allocateDeviceMemory(std::size_t n_bodies);
    void releaseDeviceMemory();
    void markHostStateDirty() noexcept;
    void uploadStateToDevice();
    void downloadStateFromDevice();
    void synchronizeDevice();
    std::size_t deviceTransferCount() const noexcept;
    bool hasDeviceMemory() const noexcept;

    void computeAccelerationsSerial();
    void computeAccelerationsParallel(int schedule_type = 0, int chunk_size = 0);

    void computeAccelerations();
    void computeAccelerations(int schedule_type);
    void computeAccelerations(int schedule_type, int chunk_size);


    /*
    * Sobrecargas CUDA requeridas por el Laboratorio 2.
    *
    * Su implementacion definitiva se conectara a la capa de
    * buffers host/device desarrollada por el Rol 2.
    */
    void computeAccelerationsGpuKernelOnly();
    void computeAccelerationsGpuKernelOnly(int variant);
    void computeAccelerationsGpuKernelOnly(int variant, int block_size);
    void computeAccelerationsGpu();
    void computeAccelerationsGpu(int variant);
    void computeAccelerationsGpu(int variant, int block_size);

    std::vector<Particle>& bodies();
    const std::vector<Particle>& bodies() const;

    double getGConst() const;
    double getSofteningEps() const;

    // Getters para acceder a los punteros crudos del device
    const double* getDeviceMass() const { return d_mass_.data(); }
    const double* getDeviceX() const { return d_x_.data(); }
    const double* getDeviceY() const { return d_y_.data(); }
    const double* getDeviceVx() const { return d_vx_.data(); }
    const double* getDeviceVy() const { return d_vy_.data(); }
};

#endif