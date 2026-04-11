#include "NBodySimulator.h"

NBodySimulator::NBodySimulator(NBodySystem& system) : system_(system) {}

void NBodySimulator::stepEuler(double dt) {
    system_.computeAccelerations();

    for (Particle& body : system_.bodies()) {
        body.kick(dt);
        body.drift(dt);
    }
}