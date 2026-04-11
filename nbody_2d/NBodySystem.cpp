#include "NBodySystem.h"

#include <cmath>
#include <cstddef>

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

void NBodySystem::computeAccelerations() {
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

        bodies_[i].addAcceleration(ax, ay);
    }
}

void NBodySystem::computeAccelerations(int schedule_type) {
    (void)schedule_type;
    computeAccelerations();
}

void NBodySystem::computeAccelerations(int schedule_type, int chunk_size) {
    (void)schedule_type;
    (void)chunk_size;
    computeAccelerations();
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