#include <iostream>
#include <cmath>
#include <cassert>
#include "../NBodySystem.h"
#include "../NBodySimulator.h"
#include "../Particle.h"

int main() {
    // 1. Configuración inicial basada en el ejemplo del enunciado
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr double dt = 0.1;
    constexpr double tolerance = 1e-4; // Tolerancia para coma flotante

    NBodySystem system(G, eps);
    // Partícula 1: masa 1, pos (0,0)
    system.addParticle(Particle(1.0, 0.0, 0.0));
    // Partícula 2: masa 1, pos (1,0)
    system.addParticle(Particle(1.0, 1.0, 0.0));

    // 2. Instanciamos el simulador
    NBodySimulator simulator(system, dt);

    // 3. Ejecutamos exactamente 1 paso de integración
    simulator.integrateEuler();

    // 4. Verificación matemática
    const std::vector<Particle>& bodies = system.bodies();
    const Particle& p1 = bodies[0];

    // Valores esperados tras 1 paso de Euler (Kick -> Drift)
    double expected_v1_x = 0.0985185; // v = a * dt
    double expected_p1_x = 0.00985185; // x = v * dt

    double diff_v = std::abs(p1.getVx() - expected_v1_x);
    double diff_x = std::abs(p1.getX() - expected_p1_x);

    bool pass_v = diff_v <= tolerance;
    bool pass_x = diff_x <= tolerance;

    std::cout << "--- TEST DE INTEGRACION (EULER) ---\n";
    std::cout << "Tiempo del simulador: " << simulator.getCurrentTime() << "\n";
    std::cout << "Velocidad P1 (x): medido=" << p1.getVx() << ", esperado~=" << expected_v1_x << " -> " << (pass_v ? "PASS" : "FAIL") << "\n";
    std::cout << "Posicion P1 (x):  medido=" << p1.getX() << ", esperado~=" << expected_p1_x << " -> " << (pass_x ? "PASS" : "FAIL") << "\n";

    if (pass_v && pass_x) {
        std::cout << "\n¡Integracion fisica correcta! El metodo de Euler funciona.\n";
        return 0;
    } else {
        std::cout << "\nError en la integracion.\n";
        return 1;
    }
}