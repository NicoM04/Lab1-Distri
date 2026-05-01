#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <iostream>

#include "../NBodySystem.h"
#include "../NBodySimulator.h"
#include "../Particle.h"

/*
Test para verificar el método de Euler.
Primero se configura un sistema con dos partículas de masa unitaria, una en el origen y otra
en (1,0). Luego se calcula la aceleración y se integra solo un paso de Euler
para después comparar los resultados con valores esperados.
*/
TEST_CASE("Integracion con metodo de Euler", "[integration][euler][nbody]") {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr double dt = 0.1;
    constexpr double tolerance = 1e-4;

    NBodySystem system(G, eps);
    system.addParticle(Particle(1.0, 0.0, 0.0));
    system.addParticle(Particle(1.0, 1.0, 0.0));

    NBodySimulator simulator(system, dt);

    // Exactamente 1 paso de integración!
    simulator.integrateEuler();

    // Verificación matem.
    const std::vector<Particle>& bodies = system.bodies();
    const Particle& p1 = bodies[0];

    // Valores esperados tras 1 paso de Euler (Kick -> Drift)
    double expected_v1_x = 0.0985185;  // v = a * dt
    double expected_p1_x = 0.00985185; // x = v * dt

    // Verificamos que la velocidad y posición.
    REQUIRE_THAT(p1.getVx(), Catch::Matchers::WithinAbs(expected_v1_x, tolerance));
    REQUIRE_THAT(p1.getX(), Catch::Matchers::WithinAbs(expected_p1_x, tolerance));
    REQUIRE_THAT(simulator.getCurrentTime(), Catch::Matchers::WithinAbs(dt, 1e-6));

    std::cout << "\nTEST DE INTEGRACION (EULER): PASS\n\n";
}