#include "NBodySystem.h"

#include <cmath>
#include <cstddef>

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
    : G_const_(G_const), softening_eps_(softening_eps) {}

void NBodySystem::addParticle(const Particle& particle) {
    bodies_.push_back(particle);
}

void NBodySystem::zeroAccelerations() {
    for (Particle& body : bodies_) {
        body.setAcceleration(0.0, 0.0);
    }
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