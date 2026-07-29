#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#include "../NBodySystem.h"
#include "../NBodySimulator.h"
#include "../Particle.h"

// Tolerancias indicadas en el enunciado para coma flotante
const double RTOL = 1e-4;
const double ATOL = 1e-8;

// Función de validación con tolerancia relativa y absoluta
bool isClose(double val, double ref) {
    return std::abs(val - ref) <= (ATOL + RTOL * std::abs(ref));
}

int main() {
    std::cout << "Iniciando test de integracion: CPU vs GPU...\n";

    const double G = 1.0;
    const double EPSILON = 0.1;
    const double DT = 0.01;
    const int NUM_STEPS = 5;

    // Instanciar dos sistemas independientes
    NBodySystem system_cpu(G, EPSILON);
    NBodySystem system_gpu(G, EPSILON);

    // Semilla fija: crear partículas iniciales idénticas para ambos sistemas
    std::vector<Particle> initial_particles = {
        Particle(1.0, 0.0, 0.0),    // Masa 1 en el origen
        Particle(2.0, 1.0, 1.0),    // Masa 2
        Particle(1.5, -1.0, 0.5)    // Masa 1.5
    };

    for (const auto& p : initial_particles) {
        system_cpu.addParticle(p);
        system_gpu.addParticle(p);
    }

    // Preparar la memoria del device para el sistema GPU
    system_gpu.allocateDeviceMemory(initial_particles.size());
    system_gpu.uploadStateToDevice();

    // Crear los simuladores
    NBodySimulator sim_cpu(system_cpu, DT);
    NBodySimulator sim_gpu(system_gpu, DT);

    // Bucle de integración (avanzar el tiempo)
    for (int i = 0; i < NUM_STEPS; ++i) {
        sim_cpu.integrateEuler(); // Baseline serial
        sim_gpu.stepEulerGpu();   // Tu nueva implementación
    }

    // Descargar el estado final (aunque stepEulerGpu ya lo hace, es buena práctica asegurarlo)
    system_gpu.downloadStateFromDevice();

    // Comparar los resultados finales
    const auto& bodies_cpu = system_cpu.bodies();
    const auto& bodies_gpu = system_gpu.bodies();

    bool all_passed = true;

    for (size_t i = 0; i < bodies_cpu.size(); ++i) {
        const auto& p_cpu = bodies_cpu[i];
        const auto& p_gpu = bodies_gpu[i];

        bool position_ok = isClose(p_gpu.getX(), p_cpu.getX()) && 
                           isClose(p_gpu.getY(), p_cpu.getY());
                           
        bool velocity_ok = isClose(p_gpu.getVx(), p_cpu.getVx()) && 
                           isClose(p_gpu.getVy(), p_cpu.getVy());

        if (!position_ok || !velocity_ok) {
            all_passed = false;
            std::cerr << "[ERROR] Discrepancia encontrada en la particula " << i << "\n"
                      << "  CPU Pos: (" << p_cpu.getX() << ", " << p_cpu.getY() << ") "
                      << "Vel: (" << p_cpu.getVx() << ", " << p_cpu.getVy() << ")\n"
                      << "  GPU Pos: (" << p_gpu.getX() << ", " << p_gpu.getY() << ") "
                      << "Vel: (" << p_gpu.getVx() << ", " << p_gpu.getVy() << ")\n";
        }
    }

    // Limpieza de memoria
    system_gpu.releaseDeviceMemory();

    if (all_passed) {
        std::cout << "[EXITO] Los resultados de CPU y GPU coinciden dentro de la tolerancia (rtol=" 
                  << RTOL << ", atol=" << ATOL << ").\n";
        return 0; // Código 0 indica éxito al CI
    } else {
        std::cerr << "[FALLO] El test de regresion no supero la prueba de tolerancia.\n";
        return 1; // Código de error para detener el CI
    }
}