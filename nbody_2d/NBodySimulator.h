#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include "Integrator.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <omp.h>

class NBodySimulator {
private:
    NBodySystem& system_;
    double time_step_;
    double current_time_;

public:
    NBodySimulator(NBodySystem& system, double time_step);
    void integrateEuler();
    void integrateEuler(int sync_type); // atomic=0, critical=1, nowait=2
    void integrateEuler(int sync_type, bool use_barrier);

    void runSimulation(int num_steps);

    void calculateEnergy();
    void calculateEnergy(int method); // reduce=0, atomic=1
    void calculateEnergy(int method, bool use_private);

    void processBodies();
    void processBodies(int task_type); // task=0, parallel_for=1
    void processBodies(int task_type, bool use_single);

    void simulatePhasesBarrier();
    void parallelInitializationSingle();

    double getCurrentTime();
};

#endif