#include "NBodySimulator.h"

NBodySimulator::NBodySimulator(NBodySystem& system, double dt) 
    : system_(system), time_step_(dt), current_time_(0.0) {}

void NBodySimulator::integrateEuler() {
    // Modo por defecto
    system_.zeroAccelerations();
    system_.computeAccelerations();
    Integrator::eulerStep(system_, time_step_, -1);
    current_time_ += time_step_;
}

void NBodySimulator::integrateEuler(int sync_type) {
    // 0=atomic, 1=critical, 2=nowait
    system_.zeroAccelerations();
    system_.computeAccelerations();
    Integrator::eulerStep(system_, time_step_, sync_type);
    current_time_ += time_step_;
}

// Para este método, el parámetro sync_type se ignora y se fuerza a usar nowait con barreras explícitas
void NBodySimulator::integrateEuler(int /*sync_type*/, bool use_barrier) {
    system_.zeroAccelerations();
    system_.computeAccelerations();
    Integrator::eulerStep(system_, time_step_, use_barrier);
    current_time_ += time_step_;
}

void NBodySimulator::runSimulation(int num_steps) {
    for (int i = 0; i < num_steps; ++i) {
        integrateEuler();
    }
}

// ------------------------------------------------------------------------------------------------- //
// Métodos para calcular energía cinética (ejemplo de uso de reduce vs atomic, y variables privadas) //
// ------------------------------------------------------------------------------------------------- //

// Variante por defecto
void NBodySimulator::calculateEnergy() {
    calculateEnergy(0, false);
}

// Variante con selección de método de sincronización
void NBodySimulator::calculateEnergy(int method) {
    calculateEnergy(method, false);
}

// Variante completa (reduce vs atomic, y uso de private)
void NBodySimulator::calculateEnergy(int method, bool use_private) {
    const std::vector<Particle>& bodies = system_.bodies();
    int n = bodies.size();
    
    double total_kinetic_energy = 0.0;

    if (use_private) {
        // private
        #pragma omp parallel
        {
            double local_K = 0.0;
            
            #pragma omp for nowait
            for (int i = 0; i < n; ++i) {
                double vx = bodies[i].getVx();
                double vy = bodies[i].getVy();
                local_K += 0.5 * bodies[i].getMass() * (vx * vx + vy * vy);
            }

            #pragma omp atomic
            total_kinetic_energy += local_K;
        }
    } 
    else if (method == 0) {
        // Reduce
        #pragma omp parallel for reduction(+:total_kinetic_energy)
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVx();
            double vy = bodies[i].getVy();
            total_kinetic_energy += 0.5 * bodies[i].getMass() * (vx * vx + vy * vy);
        }
    } 
    else if (method == 1) {
        // Atomic
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVx();
            double vy = bodies[i].getVy();
            double k_i = 0.5 * bodies[i].getMass() * (vx * vx + vy * vy);
            
            #pragma omp atomic
            total_kinetic_energy += k_i;
        }
    }
}


// -------------------------------------------------------------------------- //
// Métodos para procesar partículas (ejemplo de uso de parallel for vs tasks) //
// --------------------------------------------------------------------- ---- //

// Método processBodies por defecto con parallel for
void NBodySimulator::processBodies() {
    processBodies(1); 
}

// Variante que permite elegir entre parallel for o tasks
void NBodySimulator::processBodies(int task_type) {
    processBodies(task_type, false);
}

// Variante que permite elegir entre parallel for o tasks, y si se usa single
void NBodySimulator::processBodies(int task_type, bool use_single) {
    std::vector<Particle>& bodies = system_.bodies();
    int n = bodies.size();

    auto dummy_work = [](Particle& p) {
        double dummy = p.getMass();
        for(int k = 0; k < 100; ++k) {
            dummy = std::sin(dummy + k); // Operación para gastar CPU
        }
    };

    #pragma omp parallel
    {
        // Prueba de la directiva 'single'
        if (use_single) {
            #pragma omp single
            {
                std::cout << "Procesando " << n << " partículas con " 
                          << omp_get_num_threads() << " hilos...\n";
            }
        }

        if (task_type == 1) {
            // Usando Parallel for
            #pragma omp for
            // Cada hilo procesa un bloque de partículas
            for (int i = 0; i < n; ++i) {
                dummy_work(bodies[i]);
            }
        } 
        else if (task_type == 0) {
            // Usando Tasks
            #pragma omp single
            {
                int chunk_size = 32;
                for (int i = 0; i < n; i += chunk_size) {
                    // Cada tarea procesa un bloque de partículas
                    #pragma omp task firstprivate(i)
                    {
                        int end = std::min(i + chunk_size, n);
                        for (int j = i; j < end; ++j) {
                            dummy_work(bodies[j]);
                        }
                    }
                }
            }
        }
    }
}

// Método que simula las fases de kick y drift con barreras explícitas
void NBodySimulator::simulatePhasesBarrier() {
    // Usamos sync_type = 2 (nowait) y use_barrier = true
    integrateEuler(2, true); 
}

// Método que comprueba la inicialización paralela con una sección single
void NBodySimulator::parallelInitializationSingle() {
    #pragma omp parallel
    {
        #pragma omp single
        {
            std::cout << "Inicializando simulador con " 
                      << omp_get_num_threads() << " hilos...\n";
            std::cout << "Tiempo inicial: " << current_time_ << "\n";
        }
    }
}


// Getters
double NBodySimulator::getCurrentTime() {
    return current_time_;
}