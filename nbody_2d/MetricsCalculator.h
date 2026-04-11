#ifndef METRICSCALCULATOR_H
#define METRICSCALCULATOR_H

#include "NBodySystem.h"

struct ConservedQuantities {
    double kinetic_energy;
    double potential_energy;
    double total_momentum_x;
    double total_momentum_y;
};

class MetricsCalculator {
public:
    static ConservedQuantities compute(const NBodySystem& system);
};

#endif