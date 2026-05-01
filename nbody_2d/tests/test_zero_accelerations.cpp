#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <iostream>

#include "../NBodySystem.h"

// Test que verifica zeroAccelerations().
TEST_CASE("Poner a cero las aceleraciones", "[zero][nbody]") {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;
    constexpr double tol = 1e-15;

    NBodySystem system(G, eps);
    system.addParticle(Particle(1.0, 0.0, 0.0));
    system.addParticle(Particle(1.0, 1.0, 0.0));
    system.addParticle(Particle(1.0, 0.0, 1.0));

    system.computeAccelerations();

    // Verificamos que al menos una aceleración NO sea cero.
    bool had_nonzero = false;
    for (const auto& p : system.bodies()) {
        if (std::abs(p.getAx()) > tol || std::abs(p.getAy()) > tol) { 
            had_nonzero = true; 
            break; 
        }
    }
    
    // Si computeAccelerations() falla y no hiciera nada, el test se detiene.
    REQUIRE(had_nonzero == true);

    // Llamamos a la función que queremos probar
    system.zeroAccelerations();

    // Verificamos que todas las aceleraciones sean cero (dentro de la tolerancia)
    for (std::size_t i = 0; i < system.bodies().size(); ++i) {
        const auto& p = system.bodies()[i];
        
        INFO("Fallo al poner a cero la particula con indice: " << i);
        REQUIRE_THAT(p.getAx(), Catch::Matchers::WithinAbs(0.0, tol));
        REQUIRE_THAT(p.getAy(), Catch::Matchers::WithinAbs(0.0, tol));
    }

    std::cout << "\nTEST DE ZERO ACCELERATIONS: PASS\n\n";
}