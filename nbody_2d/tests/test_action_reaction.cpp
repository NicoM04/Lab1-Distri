#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <iostream>

#include "../NBodySystem.h"

/*
Test para la tercera ley de Newton (acción-reacción):
Trata de que para dos partículas i y j, m_i * a_i + m_j * a_j == 0 (suma vectorial)
esté dentro de una tolerancia.
*/
TEST_CASE("Tercera ley de Newton (Accion-Reaccion)", "[action_reaction][nbody]") {
    constexpr double G = 1.0;
    constexpr double eps = 1e-6;
    constexpr double tol = 1e-12;

    NBodySystem system(G, eps);
    // particula en el origen. masa m1.
    const double m1 = 1.5;
    system.addParticle(Particle(m1, 0.0, 0.0));
    // particula en x=1.masa m2.
    const double m2 = 2.3;
    system.addParticle(Particle(m2, 1.0, 0.0));

    system.computeAccelerations();

    const Particle& p0 = system.bodies().at(0);
    const Particle& p1 = system.bodies().at(1);

    // La suma ponderada de las aceleraciones por sus masas debería dar cero.
    const double sum_x = m1 * p0.getAx() + m2 * p1.getAx();
    const double sum_y = m1 * p0.getAy() + m2 * p1.getAy();

    // verifica que las sumas sean 0.0 dentro de la tolerancia
    REQUIRE_THAT(sum_x, Catch::Matchers::WithinAbs(0.0, tol));
    REQUIRE_THAT(sum_y, Catch::Matchers::WithinAbs(0.0, tol));

    std::cout << "\nTEST DE ACCION-REACCION: PASS\n\n";
}