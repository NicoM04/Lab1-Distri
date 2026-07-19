#include <iostream>
#include <vector>
#include <cmath>
#include <utility>

#include "../NBodySystem.h"
#include "../NBodySimulator.h"
#include "../Particle.h"
#include "../MetricsCalculator.h"

// Tolerancia para la comparación estática de energías (más estricta que la de integración)
const double RTOL = 1e-4;
const double ATOL = 1e-8;

bool isClose(double val, double ref) {
    return std::abs(val - ref) <= (ATOL + RTOL * std::abs(ref));
}

int main() {
    std::cout << "Iniciando test de Energias (K y U): CPU vs GPU...\n";

    const double G = 1.0;
    const double EPSILON = 0.1;
    const double DT = 0.01; // No se usará para avanzar el tiempo, solo para el simulador

    NBodySystem system(G, EPSILON);

    // Sistema de prueba con posiciones y velocidades asimétricas
    std::vector<Particle> particles = {
        Particle(1.0, 0.0, 0.0),
        Particle(2.0, 1.5, -1.0),
        Particle(0.5, -2.0, 2.5),
        Particle(1.5, 3.0, 4.0)
    };

    // Velocidades iniciales para que la Energía Cinética no sea cero
    particles[0].setVelocity(0.1, -0.2);
    particles[1].setVelocity(-0.5, 0.3);
    particles[2].setVelocity(1.0, 1.0);
    particles[3].setVelocity(-0.1, 0.0);

    for (const auto& p : particles) {
        system.addParticle(p);
    }

    // Preparar GPU
    system.allocateDeviceMemory(particles.size());
    system.uploadStateToDevice();

    NBodySimulator sim(system, DT);

    // 1. Calcular Baseline (CPU)
    ConservedQuantities cpu_metrics = MetricsCalculator::compute(system);
    double cpu_k = cpu_metrics.kinetic_energy;
    double cpu_u = cpu_metrics.potential_energy;

    std::cout << "Baseline CPU -> K: " << cpu_k << ", U: " << cpu_u << "\n";

    bool all_passed = true;

    // 2. Probar GPU Variante 0 (Reducción en Memoria Compartida)
    std::pair<double, double> gpu_metrics_reduce = sim.calculateEnergyGpu(0);
    
    if (!isClose(gpu_metrics_reduce.first, cpu_k) || !isClose(gpu_metrics_reduce.second, cpu_u)) {
        all_passed = false;
        std::cerr << "[ERROR] Discrepancia en Variante 0 (Reduccion):\n"
                  << "  GPU K: " << gpu_metrics_reduce.first << " | U: " << gpu_metrics_reduce.second << "\n";
    } else {
        std::cout << "[PASS] Variante 0 (Reduccion) coincide con CPU.\n";
    }

    // 3. Probar GPU Variante 1 (atomicAdd)
    std::pair<double, double> gpu_metrics_atomic = sim.calculateEnergyGpu(1);

    if (!isClose(gpu_metrics_atomic.first, cpu_k) || !isClose(gpu_metrics_atomic.second, cpu_u)) {
        all_passed = false;
        std::cerr << "[ERROR] Discrepancia en Variante 1 (atomicAdd):\n"
                  << "  GPU K: " << gpu_metrics_atomic.first << " | U: " << gpu_metrics_atomic.second << "\n";
    } else {
        std::cout << "[PASS] Variante 1 (atomicAdd) coincide con CPU.\n";
    }

    system.releaseDeviceMemory();

    if (all_passed) {
        std::cout << "[EXITO] Todos los calculos de energia GPU son correctos.\n";
        return 0;
    } else {
        std::cerr << "[FALLO] Existen errores en el calculo de energia GPU.\n";
        return 1;
    }
}