#include "Visualizer.h"

#include <cmath>
#include <fstream>
#include <iomanip>

#include "Integrator.h"
#include "MetricsCalculator.h"

namespace {

struct GlobalMetrics {
    double com_x;
    double com_y;
    double rms_radius;
};

// Función para calcular el centro de masa y el radio RMS del sistema
GlobalMetrics computeGlobalMetrics(const NBodySystem& system) {
    const std::vector<Particle>& bodies = system.bodies();

    double total_mass = 0.0;
    double weighted_x = 0.0;
    double weighted_y = 0.0;

    for (const Particle& p : bodies) {
        const double m = p.getMass();
        total_mass += m;
        weighted_x += m * p.getX();
        weighted_y += m * p.getY();
    }

    double com_x = 0.0;
    double com_y = 0.0;
    if (total_mass > 0.0) {
        com_x = weighted_x / total_mass;
        com_y = weighted_y / total_mass;
    }

    double weighted_r2_sum = 0.0;
    if (total_mass > 0.0) {
        for (const Particle& p : bodies) {
            const double dx = p.getX() - com_x;
            const double dy = p.getY() - com_y;
            weighted_r2_sum += p.getMass() * (dx * dx + dy * dy);
        }
    }

    double rms_radius = 0.0;
    if (total_mass > 0.0) {
        rms_radius = std::sqrt(weighted_r2_sum / total_mass);
    }

    return {com_x, com_y, rms_radius};
}

// Función para escribir un snapshot de las posiciones en CSV
void writeTrajectorySnapshot(std::ofstream& out, const NBodySystem& system, int step, double time) {
    const std::vector<Particle>& bodies = system.bodies();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const Particle& p = bodies[i];
        out << step << ' ' << std::setprecision(17) << time << ' ' << i << ' ' << p.getX() << ' ' << p.getY() << '\n';
    }
}

// Función para escribir un snapshot de las métricas globales en CSV.
void writeGlobalSnapshot(std::ofstream& out, const NBodySystem& system, int step, double time) {
    const ConservedQuantities cq = MetricsCalculator::compute(system);
    const GlobalMetrics gm = computeGlobalMetrics(system);
    const double total_energy = cq.kinetic_energy + cq.potential_energy;

    out << step << ' ' << std::setprecision(17) << time << ' ' << cq.kinetic_energy << ' ' << cq.potential_energy << ' '
        << total_energy << ' ' << gm.com_x << ' ' << gm.com_y << ' ' << gm.rms_radius << '\n';
}

}

// --- MÉTODOS DE VISUALIZER ---
// Exporta las posiciones a un CSV.
bool Visualizer::dumpPositionsCSV(const NBodySystem& system, const std::string& output_file) {
    std::ofstream out(output_file);
    if (!out.is_open()) {
        return false;
    }

    out << "id,x,y,vx,vy,ax,ay\n";
    const std::vector<Particle>& bodies = system.bodies();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const Particle& p = bodies[i];
        out << i << ',' << p.getX() << ',' << p.getY() << ',' << p.getVx() << ',' << p.getVy() << ','
            << p.getAx() << ',' << p.getAy() << '\n';
    }

    return true;
}

// Exporta las posiciones a un CSV con formato específico. Es para el analisis más que nada.
bool Visualizer::exportSimulationData(
    NBodySystem& system,
    double dt,
    const std::string& trajectories_file,
    const std::string& energy_timeseries_file,
    const SimulationExportConfig& config
) {
    if (config.num_steps < 0 || dt <= 0.0) {
        return false;
    }

    const int sample_every = config.sample_every > 0 ? config.sample_every : 1;

    std::ofstream trajectories_out(trajectories_file);
    if (!trajectories_out.is_open()) {
        return false;
    }

    trajectories_out << "# step time id x y\n";

    std::ofstream global_out;
    if (config.export_global_metrics) {
        global_out.open(energy_timeseries_file);
        if (!global_out.is_open()) {
            return false;
        }
        global_out << "# step time kinetic potential total_energy com_x com_y rms_radius\n";
    }

    if (config.include_initial_state) {
        writeTrajectorySnapshot(trajectories_out, system, 0, 0.0);
        if (config.export_global_metrics) {
            writeGlobalSnapshot(global_out, system, 0, 0.0);
        }
    }

    for (int step = 1; step <= config.num_steps; ++step) {
        system.computeAccelerations(config.schedule_type, config.chunk_size);
        Integrator::eulerStep(system, dt, -1);

        if (step % sample_every == 0) {
            const double time = static_cast<double>(step) * dt;
            writeTrajectorySnapshot(trajectories_out, system, step, time);
            if (config.export_global_metrics) {
                writeGlobalSnapshot(global_out, system, step, time);
            }
        }
    }

    return true;
}