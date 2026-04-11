#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "NBodySystem.h"

namespace Integrator {
void eulerStep(NBodySystem& system, double dt);
}

#endif