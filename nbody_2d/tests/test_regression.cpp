#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

#include "../NBodySystem.h"

/*
Test para regresión:
La aceleración de la partícula A debe escalar proporcionalmente con la masa.
Las aceleraciones de ejecuciones paralelas y seriales deben coincidir.

Hay una detección de error grave, para prevenir que el test apruebe si las aceleraciones son
por algun error, nulas (insensibilidad a la masa).
*/
TEST_CASE("Pruebas de regresion", "[regression][nbody]") {
    constexpr double G = 1.0;
    constexpr double eps = 1e-6;

    SECTION("Escalado proporcional de la aceleracion") {
        constexpr double tol_scale = 1e-12;

        NBodySystem s1(G, eps);
        NBodySystem s2(G, eps);

        // Partícula A en el origen, masa 1.0.
        s1.addParticle(Particle(1.0, 0.0, 0.0));
        s1.addParticle(Particle(1.0, 1.0, 0.0));

        s2.addParticle(Particle(1.0, 0.0, 0.0));
        s2.addParticle(Particle(2.0, 1.0, 0.0)); // Masa2 se duplica.

        s1.computeAccelerations();
        s2.computeAccelerations();

        const Particle& a1 = s1.bodies().at(0);
        const Particle& a2 = s2.bodies().at(0);

        // DETECCIÓN ERROR GRAVE
        // Si falla por el error y da todo 0.0 y el test se detiene.
        REQUIRE(std::abs(a1.getAx()) > 0.1);

        // Pq en lugar de calcular y arriesgar dividir por cero,
        // simplemente verificamos que la aceleración en s2 sea el doble que en s1.
        REQUIRE_THAT(a2.getAx(), Catch::Matchers::WithinAbs(2.0 * a1.getAx(), tol_scale));
        REQUIRE_THAT(a2.getAy(), Catch::Matchers::WithinAbs(2.0 * a1.getAy(), tol_scale));
    }

    SECTION("Paralelo vs Serial (N = 64)") {
        constexpr double tol_parallel = 1e-12;
        const std::size_t N = 64;
        
        NBodySystem serial_sys(G, eps);
        NBodySystem parallel_sys(G, eps);

        std::mt19937 rng(123);
        std::uniform_real_distribution<double> mass_dist(0.5, 2.0);
        std::uniform_real_distribution<double> pos_dist(-5.0, 5.0);

        for (std::size_t i = 0; i < N; ++i) {
            Particle p(mass_dist(rng), pos_dist(rng), pos_dist(rng));
            serial_sys.addParticle(p);
            parallel_sys.addParticle(p);
        }

        serial_sys.computeAccelerationsSerial();
        parallel_sys.computeAccelerationsParallel(0, 8); // Se usan 8 hilos

        // Comparamos partícula por partícula usando el INFO pa la trazabilidad
        for (std::size_t i = 0; i < N; ++i) {
            const Particle& ps = serial_sys.bodies().at(i);
            const Particle& pp = parallel_sys.bodies().at(i);
            
            INFO("Diferencia paralela encontrada en particula indice: " << i);
            REQUIRE_THAT(pp.getAx(), Catch::Matchers::WithinAbs(ps.getAx(), tol_parallel));
            REQUIRE_THAT(pp.getAy(), Catch::Matchers::WithinAbs(ps.getAy(), tol_parallel));
        }
    }

    std::cout << "\nTEST DE REGRESION: PASS\n\n";
}