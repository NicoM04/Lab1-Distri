#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "NBodySystem.h"

class Integrator {
public:
    static void eulerStep(NBodySystem& system, double dt, int sync_type = -1);
    static void eulerStep(NBodySystem& system, double dt, bool use_barrier);
};

#endif