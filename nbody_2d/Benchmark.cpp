#include "Benchmark.h"

#include <algorithm>
#include <chrono>
#include <random>

namespace {

void fillRandomParticles(NBodySystem& system, std::size_t n, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> mass_dist(0.5, 2.0);
    std::uniform_real_distribution<double> pos_dist(-10.0, 10.0);
    std::uniform_real_distribution<double> vel_dist(-1.0, 1.0);

    for (std::size_t i = 0; i < n; ++i) {
        Particle p(mass_dist(rng), pos_dist(rng), pos_dist(rng));
        p.setVelocity(vel_dist(rng), vel_dist(rng));
        system.addParticle(p);
    }
}

double maxAccelerationDifference(const NBodySystem& a, const NBodySystem& b) {
    const std::vector<Particle>& ab = a.bodies();
    const std::vector<Particle>& bb = b.bodies();
    const std::size_t n = std::min(ab.size(), bb.size());

    double max_diff = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double dax = std::abs(ab[i].getAx() - bb[i].getAx());
        const double day = std::abs(ab[i].getAy() - bb[i].getAy());
        max_diff = std::max(max_diff, std::max(dax, day));
    }

    return max_diff;
}

}

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

BenchmarkComparisonResult Benchmark::compareSerialVsParallel(
    std::size_t n,
    int steps,
    double dt,
    double G,
    double eps,
    int schedule_type,
    int chunk_size,
    unsigned int seed
) {
    NBodySystem serial_system(G, eps);
    NBodySystem parallel_system(G, eps);

    fillRandomParticles(serial_system, n, seed);
    for (const Particle& p : serial_system.bodies()) {
        parallel_system.addParticle(p);
    }

    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int s = 0; s < steps; ++s) {
        serial_system.computeAccelerationsSerial();
        for (Particle& body : serial_system.bodies()) {
            body.kick(dt);
            body.drift(dt);
        }
    }
    const auto t1 = std::chrono::high_resolution_clock::now();

    const auto t2 = std::chrono::high_resolution_clock::now();
    for (int s = 0; s < steps; ++s) {
        parallel_system.computeAccelerationsParallel(schedule_type, chunk_size);
        for (Particle& body : parallel_system.bodies()) {
            body.kick(dt);
            body.drift(dt);
        }
    }
    const auto t3 = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<double> serial_elapsed = t1 - t0;
    const std::chrono::duration<double> parallel_elapsed = t3 - t2;

    BenchmarkComparisonResult result{};
    result.serial_seconds = serial_elapsed.count();
    result.parallel_seconds = parallel_elapsed.count();
    result.speedup = result.parallel_seconds > 0.0 ? result.serial_seconds / result.parallel_seconds : 0.0;
    result.max_abs_acc_diff = maxAccelerationDifference(serial_system, parallel_system);
    return result;
}