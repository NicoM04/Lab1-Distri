#include "NBodySystem.h"

#include <cmath>
#include <cstddef>
#include <vector>

#include "kernels/accelerations.cuh"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

#ifdef _OPENMP
omp_sched_t mapScheduleType(int schedule_type) {
    switch (schedule_type) {
        case 1:
            return omp_sched_dynamic;
        case 2:
            return omp_sched_guided;
        case 3:
            return omp_sched_auto;
        case 0:
        default:
            return omp_sched_static;
    }
}
#endif

}

NBodySystem::NBodySystem(double G_const, double softening_eps)
    : G_const_(G_const),
      softening_eps_(softening_eps),
      device_body_count_(0U),
      device_transfer_count_(0U),
      device_mass_uploaded_(false),
      host_state_dirty_(true) {}

void NBodySystem::addParticle(const Particle& particle) {
    bodies_.push_back(particle);
    host_state_dirty_ = true;
}

void NBodySystem::zeroAccelerations() {
    for (Particle& body : bodies_) {
        body.setAcceleration(0.0, 0.0);
    }
}

void NBodySystem::ensureDeviceCapacity(std::size_t n_bodies) {
    if (device_body_count_ == n_bodies && hasDeviceMemory()) {
        return;
    }

    allocateDeviceMemory(n_bodies);
}

void NBodySystem::allocateDeviceMemory(std::size_t n_bodies) {
    device_body_count_ = n_bodies;
    device_mass_uploaded_ = false;
    host_state_dirty_ = true;

    d_mass_.allocate(n_bodies);
    d_x_.allocate(n_bodies);
    d_y_.allocate(n_bodies);
    d_vx_.allocate(n_bodies);
    d_vy_.allocate(n_bodies);
    d_ax_.allocate(n_bodies);
    d_ay_.allocate(n_bodies);
}

void NBodySystem::releaseDeviceMemory() {
    d_mass_.allocate(0);
    d_x_.allocate(0);
    d_y_.allocate(0);
    d_vx_.allocate(0);
    d_vy_.allocate(0);
    d_ax_.allocate(0);
    d_ay_.allocate(0);

    device_body_count_ = 0U;
    device_mass_uploaded_ = false;
    host_state_dirty_ = true;
}

void NBodySystem::markHostStateDirty() noexcept {
    host_state_dirty_ = true;
}

void NBodySystem::uploadStateToDevice() {
    const std::size_t n = bodies_.size();
    if (n == 0U) {
        return;
    }

    ensureDeviceCapacity(n);

    std::vector<double> mass(n);
    std::vector<double> x(n);
    std::vector<double> y(n);
    std::vector<double> vx(n);
    std::vector<double> vy(n);

    for (std::size_t i = 0; i < n; ++i) {
        const Particle& body = bodies_[i];
        mass[i] = body.getMass();
        x[i] = body.getX();
        y[i] = body.getY();
        vx[i] = body.getVx();
        vy[i] = body.getVy();
    }

    if (!device_mass_uploaded_) {
        d_mass_.copyToDevice(mass.data(), n);
        ++device_transfer_count_;
        device_mass_uploaded_ = true;
    }

    if (host_state_dirty_) {
        d_x_.copyToDevice(x.data(), n);
        d_y_.copyToDevice(y.data(), n);
        d_vx_.copyToDevice(vx.data(), n);
        d_vy_.copyToDevice(vy.data(), n);
        device_transfer_count_ += 4U;
    }

    host_state_dirty_ = false;
}

void NBodySystem::downloadStateFromDevice() {
    const std::size_t n = bodies_.size();
    if (n == 0U || !hasDeviceMemory()) {
        return;
    }

    std::vector<double> ax(n);
    std::vector<double> ay(n);

    d_ax_.copyToHost(ax.data(), n);
    d_ay_.copyToHost(ay.data(), n);
    device_transfer_count_ += 2U;

    for (std::size_t i = 0; i < n; ++i) {
        bodies_[i].setAcceleration(ax[i], ay[i]);
    }
}

void NBodySystem::synchronizeDevice() {
    CUDA_CHECK(cudaDeviceSynchronize());
}

std::size_t NBodySystem::deviceTransferCount() const noexcept {
    return device_transfer_count_;
}

bool NBodySystem::hasDeviceMemory() const noexcept {
    return d_mass_.size() == device_body_count_
        && d_x_.size() == device_body_count_
        && d_y_.size() == device_body_count_
        && d_vx_.size() == device_body_count_
        && d_vy_.size() == device_body_count_
    && d_ax_.size() == device_body_count_
    && d_ay_.size() == device_body_count_;
}

void NBodySystem::computeAccelerationsSerial() {
    zeroAccelerations();

    const std::size_t n = bodies_.size();
    const double eps2 = softening_eps_ * softening_eps_;

    for (std::size_t i = 0; i < n; ++i) {
        double ax = 0.0;
        double ay = 0.0;

        const double xi = bodies_[i].getX();
        const double yi = bodies_[i].getY();

        for (std::size_t j = 0; j < n; ++j) {
            if (i == j) {
                continue;
            }

            const double dx = bodies_[j].getX() - xi;
            const double dy = bodies_[j].getY() - yi;
            const double dist2 = dx * dx + dy * dy + eps2;

            const double inv_dist = 1.0 / std::sqrt(dist2);
            const double inv_dist3 = inv_dist * inv_dist * inv_dist;
            const double factor = G_const_ * bodies_[j].getMass() * inv_dist3;

            ax += factor * dx;
            ay += factor * dy;
        }

        bodies_[i].setAcceleration(ax, ay);
    }
}

void NBodySystem::computeAccelerationsParallel(int schedule_type, int chunk_size) {
#ifndef _OPENMP
    (void)schedule_type;
    (void)chunk_size;
    computeAccelerationsSerial();
#else
    zeroAccelerations();

    const std::size_t n = bodies_.size();
    const double eps2 = softening_eps_ * softening_eps_;

    const omp_sched_t omp_schedule = mapScheduleType(schedule_type);
    const int effective_chunk = chunk_size > 0 ? chunk_size : 1;
    omp_set_schedule(omp_schedule, effective_chunk);

#pragma omp parallel for schedule(runtime)
    for (long long i = 0; i < static_cast<long long>(n); ++i) {
        double ax = 0.0;
        double ay = 0.0;

        const double xi = bodies_[static_cast<std::size_t>(i)].getX();
        const double yi = bodies_[static_cast<std::size_t>(i)].getY();

        for (std::size_t j = 0; j < n; ++j) {
            if (static_cast<std::size_t>(i) == j) {
                continue;
            }

            const double dx = bodies_[j].getX() - xi;
            const double dy = bodies_[j].getY() - yi;
            const double dist2 = dx * dx + dy * dy + eps2;

            const double inv_dist = 1.0 / std::sqrt(dist2);
            const double inv_dist3 = inv_dist * inv_dist * inv_dist;
            const double factor = G_const_ * bodies_[j].getMass() * inv_dist3;

            ax += factor * dx;
            ay += factor * dy;
        }

        bodies_[static_cast<std::size_t>(i)].setAcceleration(ax, ay);
    }
#endif
}

void NBodySystem::computeAccelerations() {
    computeAccelerationsParallel(0, 0);
}

void NBodySystem::computeAccelerations(int schedule_type) {
    computeAccelerationsParallel(schedule_type, 0);
}

void NBodySystem::computeAccelerations(int schedule_type, int chunk_size) {
    computeAccelerationsParallel(schedule_type, chunk_size);
}

void NBodySystem::computeAccelerationsGpu() {
    computeAccelerationsGpuKernelOnly(0, 256);
    synchronizeDevice();
    downloadStateFromDevice();
}

void NBodySystem::computeAccelerationsGpuKernelOnly() {
    computeAccelerationsGpuKernelOnly(0, 256);
}

void NBodySystem::computeAccelerationsGpuKernelOnly(int variant) {
    computeAccelerationsGpuKernelOnly(variant, 256);
}

void NBodySystem::computeAccelerationsGpuKernelOnly(int variant, int block_size) {
    const std::size_t n = bodies_.size();
    if (n == 0U) {
        return;
    }

    if (!hasDeviceMemory() || host_state_dirty_) {
        uploadStateToDevice();
    }

    launchComputeAccelerations(
        d_mass_.data(),
        d_x_.data(),
        d_y_.data(),
        d_ax_.data(),
        d_ay_.data(),
        n,
        G_const_,
        softening_eps_,
        variant,
        block_size
    );
}

void NBodySystem::computeAccelerationsGpu(int variant) {
    computeAccelerationsGpuKernelOnly(variant, 256);
    synchronizeDevice();
    downloadStateFromDevice();
}

void NBodySystem::computeAccelerationsGpu(int variant, int block_size) {
    computeAccelerationsGpuKernelOnly(variant, block_size);
    synchronizeDevice();
    downloadStateFromDevice();
}

std::vector<Particle>& NBodySystem::bodies() {
    return bodies_;
}

const std::vector<Particle>& NBodySystem::bodies() const {
    return bodies_;
}

double NBodySystem::getGConst() const {
    return G_const_;
}

double NBodySystem::getSofteningEps() const {
    return softening_eps_;
}