#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "NBodySystem.h"

class Benchmark {
public:
    static double runSteps(NBodySystem& system, int steps, double dt);
};

#endif