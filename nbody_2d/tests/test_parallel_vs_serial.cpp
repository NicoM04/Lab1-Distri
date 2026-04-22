#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

#include "../NBodySystem.h"

int main() {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr std::size_t N = 32;
    constexpr double tolerance = 1e-12;

    NBodySystem serial_system(G, eps);
    NBodySystem parallel_system(G, eps);

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> mass_dist(0.5, 2.0);
    std::uniform_real_distribution<double> pos_dist(-5.0, 5.0);

    for (std::size_t i = 0; i < N; ++i) {
        Particle p(mass_dist(rng), pos_dist(rng), pos_dist(rng));
        serial_system.addParticle(p);
        parallel_system.addParticle(p);
    }

    serial_system.computeAccelerationsSerial();
    parallel_system.computeAccelerationsParallel(0, 4);

    double max_abs_diff = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        const Particle& ps = serial_system.bodies().at(i);
        const Particle& pp = parallel_system.bodies().at(i);

        const double dax = std::abs(ps.getAx() - pp.getAx());
        const double day = std::abs(ps.getAy() - pp.getAy());
        max_abs_diff = std::max(max_abs_diff, std::max(dax, day));
    }

    const bool pass = max_abs_diff <= tolerance;
    std::cout << "N=" << N << " max_abs_diff=" << max_abs_diff
              << " tolerance=" << tolerance
              << " pass=" << pass << "\n";
    return pass ? 0 : 1;
}
