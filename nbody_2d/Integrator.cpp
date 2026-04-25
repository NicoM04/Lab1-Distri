#include "Integrator.h"

void Integrator::eulerStep(NBodySystem& system, double dt, int sync_type) {
    std::vector<Particle>& bodies = system.bodies();
    int n = bodies.size();

    if (sync_type == 2) {
        // 2 = NOWAIT
        #pragma omp parallel
        {
            #pragma omp for nowait
            for (int i = 0; i < n; ++i) {
                bodies[i].kick(dt);
            }
            
            #pragma omp for
            for (int i = 0; i < n; ++i) {
                bodies[i].drift(dt);
            }
        }
    } 

    else if (sync_type == 1) {
        // 1 = CRITICAL
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            #pragma omp critical
            {
                bodies[i].kick(dt);
                bodies[i].drift(dt);
            }
        }
    } 
    else if (sync_type == 0) {
        // 0 = ATOMIC
        int operaciones_realizadas = 0;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            bodies[i].kick(dt);
            bodies[i].drift(dt);

            #pragma omp atomic
            operaciones_realizadas++; 
        }
    } 
    else {
        // MODO NORMAL PARALELO
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            bodies[i].kick(dt);
        }

        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            bodies[i].drift(dt);
        }
    }
}

void Integrator::eulerStep(NBodySystem& system, double dt, bool use_barrier) {
    std::vector<Particle>& bodies = system.bodies();
    int n = bodies.size();

    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int i = 0; i < n; ++i) {
            bodies[i].kick(dt);
        }

        // Sincronización explícita
        if (use_barrier) {
            #pragma omp barrier
        }

        #pragma omp for nowait
        for (int i = 0; i < n; ++i) {
            bodies[i].drift(dt);
        }
    }
}