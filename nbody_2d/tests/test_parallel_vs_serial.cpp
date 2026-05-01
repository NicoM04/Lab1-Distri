#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

#include "../NBodySystem.h"

/*
Test para comparar aceleraciones serial vs paralelo:
Se usan condiciones iniciales aleatorias pero de tal forma que puedan ser reproducibles.
El test falla si las implementaciones difieren de una pequeña tolerancia.
*/
TEST_CASE("Comparacion aceleraciones serial vs paralelo", "[parallel][regression][nbody]") {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr std::size_t N = 32;
    constexpr double tolerance = 1e-12;

    // Creamos dos sistemas iguales
    NBodySystem serial_system(G, eps);
    NBodySystem parallel_system(G, eps);

    // Usamos un "generador de números" aleatorios pero con una semilla fija para la reproducibilidad.
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> mass_dist(0.5, 2.0);
    std::uniform_real_distribution<double> pos_dist(-5.0, 5.0);

    // Creamos las mismas partículas en ambos sistemas.
    for (std::size_t i = 0; i < N; ++i) {
        Particle p(mass_dist(rng), pos_dist(rng), pos_dist(rng));
        serial_system.addParticle(p);
        parallel_system.addParticle(p);
    }

    serial_system.computeAccelerationsSerial();
    parallel_system.computeAccelerationsParallel(0, 4);

    // Comparamos aceleraciones partícula por partícula
    for (std::size_t i = 0; i < N; ++i) {
        const Particle& ps = serial_system.bodies().at(i);
        const Particle& pp = parallel_system.bodies().at(i);

        // INFO registra un mensaje,que solo se se muestra si REQUIRE falla.
        INFO("Diferencia encontrada en la particula con indice: " << i);
        
        // Comparamos que la aceleraciones.
        REQUIRE_THAT(pp.getAx(), Catch::Matchers::WithinAbs(ps.getAx(), tolerance));
        REQUIRE_THAT(pp.getAy(), Catch::Matchers::WithinAbs(ps.getAy(), tolerance));
    }

    std::cout << "\nTEST DE REGRESION (PARALLEL VS SERIAL): PASS\n\n";
}