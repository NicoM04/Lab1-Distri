#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp> // para comparar doubles
#include <cmath>
#include <iostream>

#include "../NBodySystem.h"

/*
 Test para probar la magnitud de la aceleración.
 Dos partículas de masa unitaria en (0,0) y (1,0).
 Se calcula la aceleración para luego verificar que la magnitud en la primera
 partícula esté dentro de una tolerancia razonable.
*/
TEST_CASE("Magnitud de la aceleracion", "[acceleration][nbody]") {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr double expected = 0.971;
    constexpr double tolerance = 0.02;

    NBodySystem system(G, eps);
    system.addParticle(Particle(1.0, 0.0, 0.0));
    system.addParticle(Particle(1.0, 1.0, 0.0));
    system.computeAccelerations();

    const Particle& p0 = system.bodies().at(0);
    const double magnitude = std::sqrt(p0.getAx() * p0.getAx() + p0.getAy() * p0.getAy());

    REQUIRE_THAT(magnitude, Catch::Matchers::WithinAbs(expected, tolerance));

    std::cout << "\nTEST DE ACELERACION: PASS\n\n";
}