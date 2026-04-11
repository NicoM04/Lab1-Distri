#include "MetricsCalculator.h"

#include <cmath>
#include <cstddef>

ConservedQuantities MetricsCalculator::compute(const NBodySystem& system) {
    ConservedQuantities result{0.0, 0.0, 0.0, 0.0};

    const std::vector<Particle>& bodies = system.bodies();
    const std::size_t n = bodies.size();

    for (const Particle& p : bodies) {
        const double v2 = p.getVx() * p.getVx() + p.getVy() * p.getVy();
        result.kinetic_energy += 0.5 * p.getMass() * v2;
        result.total_momentum_x += p.getMass() * p.getVx();
        result.total_momentum_y += p.getMass() * p.getVy();
    }

    const double G = system.getGConst();
    const double eps2 = system.getSofteningEps() * system.getSofteningEps();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const double dx = bodies[j].getX() - bodies[i].getX();
            const double dy = bodies[j].getY() - bodies[i].getY();
            const double dist = std::sqrt(dx * dx + dy * dy + eps2);
            result.potential_energy += -G * bodies[i].getMass() * bodies[j].getMass() / dist;
        }
    }

    return result;
}