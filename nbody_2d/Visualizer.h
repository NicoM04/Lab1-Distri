#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <string>

#include "NBodySystem.h"

struct SimulationExportConfig {
    int num_steps = 0;
    int sample_every = 1;
    int schedule_type = 0;
    int chunk_size = 0;
    bool include_initial_state = true;
    bool export_global_metrics = false;
};

class Visualizer {
public:
    static bool dumpPositionsCSV(const NBodySystem& system, const std::string& output_file);
    static bool exportSimulationData(
        NBodySystem& system,
        double dt,
        const std::string& trajectories_file,
        const std::string& energy_timeseries_file,
        const SimulationExportConfig& config
    );
};

#endif