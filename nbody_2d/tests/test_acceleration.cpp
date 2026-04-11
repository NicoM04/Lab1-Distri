#include <cmath>
#include <iostream>

#include "../NBodySystem.h"

int main() {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr double expected = 0.971;
    constexpr double tolerance = 0.02;

    NBodySystem system(G, eps);
    system.addParticle(Particle(1.0, 0.0, 0.0));
    system.addParticle(Particle(1.0, 1.0, 0.0));
    system.computeAccelerations();

    const Particle& p0 = system.bodies().at(0);
    const double magnitude = std::sqrt(p0.getAx() * p0.getAx() + p0.getAy() * p0.getAy());

    const bool pass = std::abs(magnitude - expected) <= tolerance;
    std::cout << "measured=" << magnitude << " expected~=" << expected << " pass=" << pass << "\n";
    return pass ? 0 : 1;
}