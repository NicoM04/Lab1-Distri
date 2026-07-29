#ifndef NBODYSYSTEM_H
#define NBODYSYSTEM_H

#include <vector>

#include "Particle.h"

class NBodySystem {
private:
    std::vector<Particle> bodies_;
    double G_const_;
    double softening_eps_;

public:
    NBodySystem(double G_const, double softening_eps);

    void addParticle(const Particle& particle);
    void zeroAccelerations();

    void computeAccelerationsSerial();
    void computeAccelerationsParallel(int schedule_type = 0, int chunk_size = 0);

    void computeAccelerations();
    void computeAccelerations(int schedule_type);
    void computeAccelerations(int schedule_type, int chunk_size);


    /*
    * Sobrecargas CUDA requeridas por el Laboratorio 2.
    *
    * Su implementacion definitiva se conectara a la capa de
    * buffers host/device desarrollada por el Rol 2.
    */
    void computeAccelerationsGpu();
    void computeAccelerationsGpu(int variant);
    void computeAccelerationsGpu(int variant, int block_size);

    std::vector<Particle>& bodies();
    const std::vector<Particle>& bodies() const;

    double getGConst() const;
    double getSofteningEps() const;
};

#endif