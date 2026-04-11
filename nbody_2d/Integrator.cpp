#include "Integrator.h"

namespace Integrator {

void eulerStep(NBodySystem& system, double dt) {
    system.computeAccelerations();
    for (Particle& body : system.bodies()) {
        body.kick(dt);
        body.drift(dt);
    }
}

}