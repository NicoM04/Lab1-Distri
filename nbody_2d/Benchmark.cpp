#include "Benchmark.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <random>

#include "Integrator.h"

#ifdef _OPENMP
#include <omp.h>
#endif

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

std::vector<Particle> makeRandomParticles(std::size_t n, unsigned int seed) {
    std::vector<Particle> particles;
    particles.reserve(n);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> mass_dist(0.5, 2.0);
    std::uniform_real_distribution<double> pos_dist(-10.0, 10.0);
    std::uniform_real_distribution<double> vel_dist(-1.0, 1.0);

    for (std::size_t i = 0; i < n; ++i) {
        Particle p(mass_dist(rng), pos_dist(rng), pos_dist(rng));
        p.setVelocity(vel_dist(rng), vel_dist(rng));
        particles.push_back(p);
    }

    return particles;
}

NBodySystem makeSystemFromParticles(const std::vector<Particle>& particles, double G, double eps) {
    NBodySystem system(G, eps);
    for (const Particle& p : particles) {
        system.addParticle(p);
    }
    return system;
}

BenchmarkStats computeStats(const std::vector<double>& values) {
    BenchmarkStats stats{0.0, 0.0};
    if (values.empty()) {
        return stats;
    }

    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    stats.mean = sum / static_cast<double>(values.size());

    if (values.size() == 1U) {
        return stats;
    }

    double sq_sum = 0.0;
    for (double value : values) {
        const double d = value - stats.mean;
        sq_sum += d * d;
    }
    stats.stddev = std::sqrt(sq_sum / static_cast<double>(values.size() - 1U));
    return stats;
}

double wallTimeSeconds() {
#ifdef _OPENMP
    return omp_get_wtime();
#else
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
#endif
}

void setNumThreads(int threads) {
#ifdef _OPENMP
    if (threads > 0) {
        omp_set_num_threads(threads);
    }
#else
    (void)threads;
#endif
}

int effectiveChunkSize(int chunk_size) {
    return chunk_size > 0 ? chunk_size : 1;
}

void applySpeedupAndEfficiency(std::vector<BenchmarkRunResult>& results, bool use_full_step_time) {
    if (results.empty()) {
        return;
    }

    auto baseline_it = std::find_if(results.begin(), results.end(), [](const BenchmarkRunResult& r) {
        return r.threads == 1;
    });

    if (baseline_it == results.end()) {
        return;
    }

    const double t1 = use_full_step_time ? baseline_it->full_step_time.mean : baseline_it->compute_time.mean;
    const double sigma_t1 = use_full_step_time ? baseline_it->full_step_time.stddev : baseline_it->compute_time.stddev;

    if (t1 <= 0.0) {
        return;
    }

    for (BenchmarkRunResult& r : results) {
        const double tp = use_full_step_time ? r.full_step_time.mean : r.compute_time.mean;
        const double sigma_tp = use_full_step_time ? r.full_step_time.stddev : r.compute_time.stddev;

        if (tp <= 0.0 || r.threads <= 0) {
            continue;
        }

        const double sp = t1 / tp;
        const double ep = sp / static_cast<double>(r.threads);

        double sigma_sp = 0.0;
        if (sigma_t1 > 0.0 || sigma_tp > 0.0) {
            const double rel_t1 = sigma_t1 / t1;
            const double rel_tp = sigma_tp / tp;
            sigma_sp = sp * std::sqrt(rel_t1 * rel_t1 + rel_tp * rel_tp);
        }

        const double sigma_ep = sigma_sp / static_cast<double>(r.threads);

        if (use_full_step_time) {
            r.full_step_speedup = sp;
            r.full_step_efficiency = ep;
            r.full_step_speedup_std = sigma_sp;
            r.full_step_efficiency_std = sigma_ep;
        } else {
            r.compute_speedup = sp;
            r.compute_efficiency = ep;
            r.compute_speedup_std = sigma_sp;
            r.compute_efficiency_std = sigma_ep;
        }
    }
}

double estimateSerialFraction(double t1, double tp, int threads) {
    if (t1 <= 0.0 || tp <= 0.0 || threads <= 1) {
        return 0.0;
    }

    const double speedup = t1 / tp;
    if (speedup <= 0.0) {
        return 0.0;
    }

    const double p = static_cast<double>(threads);
    const double numerator = (1.0 / speedup) - (1.0 / p);
    const double denominator = 1.0 - (1.0 / p);
    if (denominator <= 0.0) {
        return 0.0;
    }

    const double f = numerator / denominator;
    if (f < 0.0) {
        return 0.0;
    }
    if (f > 1.0) {
        return 1.0;
    }
    return f;
}

AmdahlResult buildAmdahlResult(int threads, double measured_speedup, double serial_fraction) {
    AmdahlResult result{};
    result.threads = threads;
    result.measured_speedup = measured_speedup;
    result.serial_fraction = serial_fraction;

    if (threads > 0) {
        const double p = static_cast<double>(threads);
        result.amdahl_speedup = 1.0 / (serial_fraction + (1.0 - serial_fraction) / p);
    }

    return result;
}

std::vector<AmdahlResult> computeAmdahlResults(
    const std::vector<BenchmarkRunResult>& results,
    bool use_full_step_time
) {
    std::vector<AmdahlResult> amdahl_results;
    if (results.empty()) {
        return amdahl_results;
    }

    auto baseline_it = std::find_if(results.begin(), results.end(), [](const BenchmarkRunResult& result) {
        return result.threads == 1;
    });

    if (baseline_it == results.end()) {
        return amdahl_results;
    }

    const double t1 = use_full_step_time ? baseline_it->full_step_time.mean : baseline_it->compute_time.mean;
    if (t1 <= 0.0) {
        return amdahl_results;
    }

    double serial_fraction_sum = 0.0;
    int serial_fraction_count = 0;

    for (const BenchmarkRunResult& result : results) {
        const double tp = use_full_step_time ? result.full_step_time.mean : result.compute_time.mean;
        if (result.threads <= 1 || tp <= 0.0) {
            continue;
        }

        const double serial_fraction = estimateSerialFraction(t1, tp, result.threads);
        serial_fraction_sum += serial_fraction;
        ++serial_fraction_count;
    }

    const double mean_serial_fraction = serial_fraction_count > 0
        ? serial_fraction_sum / static_cast<double>(serial_fraction_count)
        : 0.0;

    amdahl_results.reserve(results.size());
    for (const BenchmarkRunResult& result : results) {
        const double tp = use_full_step_time ? result.full_step_time.mean : result.compute_time.mean;
        const double measured_speedup = tp > 0.0 ? t1 / tp : 0.0;
        amdahl_results.push_back(buildAmdahlResult(result.threads, measured_speedup, mean_serial_fraction));
    }

    return amdahl_results;
}

}

double Benchmark::measureComputeAccelerationsTime(NBodySystem& system, int schedule_type, int chunk_size) {
    const double t0 = wallTimeSeconds();
    system.computeAccelerationsParallel(schedule_type, effectiveChunkSize(chunk_size));
    const double t1 = wallTimeSeconds();
    return t1 - t0;
}

double Benchmark::measureFullStepTime(NBodySystem& system, double dt, int schedule_type, int chunk_size) {
    const double t0 = wallTimeSeconds();
    system.computeAccelerationsParallel(schedule_type, effectiveChunkSize(chunk_size));
    Integrator::eulerStep(system, dt, -1);
    const double t1 = wallTimeSeconds();
    return t1 - t0;
}

BenchmarkRunResult Benchmark::runExperiment(
    std::size_t n,
    int threads,
    int repetitions,
    double dt,
    double G,
    double eps,
    int schedule_type,
    int chunk_size,
    unsigned int seed
) {
    BenchmarkRunResult result{};
    result.threads = threads;
    result.problem_size = n;
    result.schedule_type = schedule_type;
    result.chunk_size = chunk_size;
    result.repetitions = repetitions;

    const std::vector<Particle> initial_particles = makeRandomParticles(n, seed);

    setNumThreads(threads);

    const int reps = std::max(1, repetitions);
    std::vector<double> compute_times;
    std::vector<double> full_step_times;
    compute_times.reserve(static_cast<std::size_t>(reps));
    full_step_times.reserve(static_cast<std::size_t>(reps));

    for (int r = 0; r < reps; ++r) {
        NBodySystem compute_system = makeSystemFromParticles(initial_particles, G, eps);
        NBodySystem full_step_system = makeSystemFromParticles(initial_particles, G, eps);

        compute_times.push_back(measureComputeAccelerationsTime(compute_system, schedule_type, chunk_size));
        full_step_times.push_back(measureFullStepTime(full_step_system, dt, schedule_type, chunk_size));
    }

    result.compute_time = computeStats(compute_times);
    result.full_step_time = computeStats(full_step_times);
    return result;
}

std::vector<BenchmarkRunResult> Benchmark::runScalingAnalysis(
    std::size_t n,
    const std::vector<int>& thread_counts,
    int repetitions,
    double dt,
    double G,
    double eps,
    int schedule_type,
    int chunk_size,
    unsigned int seed
) {
    std::vector<BenchmarkRunResult> results;
    results.reserve(thread_counts.size());

    for (int threads : thread_counts) {
        if (threads <= 0) {
            continue;
        }
        results.push_back(runExperiment(n, threads, repetitions, dt, G, eps, schedule_type, chunk_size, seed));
    }

    applySpeedupAndEfficiency(results, false);
    applySpeedupAndEfficiency(results, true);
    return results;
}

std::vector<BenchmarkRunResult> Benchmark::runScheduleComparison(
    std::size_t n,
    int threads,
    int repetitions,
    double dt,
    double G,
    double eps,
    const std::vector<int>& schedule_types,
    const std::vector<int>& chunk_sizes,
    unsigned int seed
) {
    std::vector<BenchmarkRunResult> results;

    for (int schedule_type : schedule_types) {
        if (schedule_type < 0 || schedule_type > 2) {
            continue;
        }

        for (int chunk_size : chunk_sizes) {
            if (chunk_size <= 0) {
                continue;
            }

            results.push_back(runExperiment(n, threads, repetitions, dt, G, eps, schedule_type, chunk_size, seed));
        }
    }

    return results;
}

bool Benchmark::exportScalingDat(
    const std::string& output_path,
    const std::vector<BenchmarkRunResult>& results,
    bool use_full_step_time
) {
    const std::vector<AmdahlResult> amdahl_results = computeAmdahlResults(results, use_full_step_time);

    std::ofstream out(output_path);
    if (!out) {
        return false;
    }

    out << std::fixed << std::setprecision(8);
    out << "# threads time_mean time_std speedup efficiency amdahl_speedup\n";

    for (std::size_t i = 0; i < results.size(); ++i) {
        const BenchmarkRunResult& r = results[i];
        const double mean = use_full_step_time ? r.full_step_time.mean : r.compute_time.mean;
        const double stddev = use_full_step_time ? r.full_step_time.stddev : r.compute_time.stddev;
        const double speedup = use_full_step_time ? r.full_step_speedup : r.compute_speedup;
        const double efficiency = use_full_step_time ? r.full_step_efficiency : r.compute_efficiency;
        const double amdahl_speedup = i < amdahl_results.size() ? amdahl_results[i].amdahl_speedup : 0.0;

        out << r.threads << ' '
            << mean << ' '
            << stddev << ' '
            << speedup << ' '
            << efficiency << ' '
            << amdahl_speedup << '\n';
    }

    return true;
}

bool Benchmark::exportScheduleDat(
    const std::string& output_path,
    const std::vector<BenchmarkRunResult>& results,
    bool use_full_step_time
) {
    std::ofstream out(output_path);
    if (!out) {
        return false;
    }

    out << std::fixed << std::setprecision(8);
    out << "# schedule chunk threads time_mean time_std\n";

    for (const BenchmarkRunResult& r : results) {
        const double mean = use_full_step_time ? r.full_step_time.mean : r.compute_time.mean;
        const double stddev = use_full_step_time ? r.full_step_time.stddev : r.compute_time.stddev;

        out << scheduleName(r.schedule_type) << ' '
            << r.chunk_size << ' '
            << r.threads << ' '
            << mean << ' '
            << stddev << '\n';
    }

    return true;
}

std::vector<AmdahlResult> Benchmark::runAmdahlAnalysis(
    const std::vector<BenchmarkRunResult>& results,
    bool use_full_step_time
) {
    return computeAmdahlResults(results, use_full_step_time);
}

bool Benchmark::exportAmdahlDat(
    const std::string& output_path,
    const std::vector<AmdahlResult>& results
) {
    std::ofstream out(output_path);
    if (!out) {
        return false;
    }

    out << std::fixed << std::setprecision(8);
    out << "# threads measured_speedup amdahl_speedup serial_fraction\n";

    for (const AmdahlResult& result : results) {
        out << result.threads << ' '
            << result.measured_speedup << ' '
            << result.amdahl_speedup << ' '
            << result.serial_fraction << '\n';
    }

    return true;
}

const char* Benchmark::scheduleName(int schedule_type) {
    switch (schedule_type) {
        case 1:
            return "dynamic";
        case 2:
            return "guided";
        case 0:
        default:
            return "static";
    }
}

double Benchmark::runSteps(NBodySystem& system, int steps, double dt) {
    const int effective_steps = std::max(0, steps);
    const double t0 = wallTimeSeconds();

    for (int s = 0; s < effective_steps; ++s) {
        system.computeAccelerations();
        Integrator::eulerStep(system, dt, -1);
    }

    const double t1 = wallTimeSeconds();
    return t1 - t0;
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

    const double t0 = wallTimeSeconds();
    for (int s = 0; s < steps; ++s) {
        serial_system.computeAccelerationsSerial();
        for (Particle& body : serial_system.bodies()) {
            body.kick(dt);
            body.drift(dt);
        }
    }
    const double t1 = wallTimeSeconds();

    const double t2 = wallTimeSeconds();
    for (int s = 0; s < steps; ++s) {
        parallel_system.computeAccelerationsParallel(schedule_type, chunk_size);
        for (Particle& body : parallel_system.bodies()) {
            body.kick(dt);
            body.drift(dt);
        }
    }
    const double t3 = wallTimeSeconds();

    BenchmarkComparisonResult result{};
    result.serial_seconds = t1 - t0;
    result.parallel_seconds = t3 - t2;
    result.speedup = result.parallel_seconds > 0.0 ? result.serial_seconds / result.parallel_seconds : 0.0;
    result.max_abs_acc_diff = maxAccelerationDifference(serial_system, parallel_system);
    return result;
}