#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"

class NBodySimulator {
private:
    NBodySystem& system_;

public:
    explicit NBodySimulator(NBodySystem& system);
    void stepEuler(double dt);
};

#endif