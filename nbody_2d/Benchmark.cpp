#include "Benchmark.h"

#include <chrono>

double Benchmark::runSteps(NBodySystem& system, int steps, double dt) {
    const auto t0 = std::chrono::high_resolution_clock::now();

    for (int s = 0; s < steps; ++s) {
        system.computeAccelerations();
        for (Particle& body : system.bodies()) {
            body.kick(dt);
            body.drift(dt);
        }
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = t1 - t0;
    return elapsed.count();
}