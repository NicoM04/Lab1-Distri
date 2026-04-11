#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "NBodySystem.h"

namespace {

void printAccelerationSummary(const NBodySystem& system) {
    const std::vector<Particle>& bodies = system.bodies();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const Particle& p = bodies[i];
        std::cout << "Body " << i << ": a = (" << p.getAx() << ", " << p.getAy() << ")\n";
    }
}

}

int main() {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;

    std::cout << std::fixed << std::setprecision(6);

    NBodySystem system(G, eps);
    system.addParticle(Particle(1.0, 0.0, 0.0));
    system.addParticle(Particle(1.0, 1.0, 0.0));
    system.addParticle(Particle(1.0, 0.0, 1.0));

    system.computeAccelerations();
    std::cout << "N=3 acceleration snapshot:\n";
    printAccelerationSummary(system);

    NBodySystem pair_system(G, eps);
    pair_system.addParticle(Particle(1.0, 0.0, 0.0));
    pair_system.addParticle(Particle(1.0, 1.0, 0.0));
    pair_system.computeAccelerations();

    const Particle& p0 = pair_system.bodies().at(0);
    const double measured_a = std::sqrt(p0.getAx() * p0.getAx() + p0.getAy() * p0.getAy());

    constexpr double expected_a = 0.971;
    constexpr double tolerance = 0.02;
    const bool pass = std::abs(measured_a - expected_a) <= tolerance;

    std::cout << "\nValidation (d=1, m=1, G=1, eps=0.1):\n";
    std::cout << "  measured |a| = " << measured_a << "\n";
    std::cout << "  expected ~= " << expected_a << " (statement reference)\n";
    std::cout << "  tolerance = " << tolerance << "\n";
    std::cout << "  status    = " << (pass ? "PASS" : "FAIL") << "\n";

    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}