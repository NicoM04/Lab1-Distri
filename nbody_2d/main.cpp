#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <cstdio>
#include <string>
#include <vector>

#include "Benchmark.h"
#include "Visualizer.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

bool hasFlag(const std::vector<std::string>& args, const std::string& flag) {
    return std::find(args.begin(), args.end(), flag) != args.end();
}

bool getArgValue(const std::vector<std::string>& args, const std::string& key, std::string& value) {
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == key) {
            value = args[i + 1];
            return true;
        }
    }
    return false;
}

std::vector<int> parseIntCsv(const std::string& csv) {
    std::vector<int> values;
    std::stringstream ss(csv);
    std::string token;

    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            values.push_back(std::stoi(token));
        }
    }

    return values;
}

std::vector<int> sortUniquePositive(std::vector<int> values) {
    values.erase(std::remove_if(values.begin(), values.end(), [](int value) {
        return value <= 0;
    }), values.end());

    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

int scheduleFromString(const std::string& schedule_name) {
    if (schedule_name == "dynamic") {
        return 1;
    }
    if (schedule_name == "guided") {
        return 2;
    }
    return 0;
}

int maxThreads() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

NBodySystem buildRandomSystem(std::size_t n, double G, double eps, unsigned int seed) {
    NBodySystem system(G, eps);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> mass_dist(0.5, 2.0);
    std::uniform_real_distribution<double> pos_dist(-10.0, 10.0);
    std::uniform_real_distribution<double> vel_dist(-1.0, 1.0);

    for (std::size_t i = 0; i < n; ++i) {
        Particle p(mass_dist(rng), pos_dist(rng), pos_dist(rng));
        p.setVelocity(vel_dist(rng), vel_dist(rng));
        system.addParticle(p);
    }

    return system;
}

std::string quoteCommandArg(const std::string& value) {
    return "\"" + value + "\"";
}

bool runPlotScript(const std::string& script_path, const std::string& dat_path, const std::string& output_prefix) {
    const std::string png_path = output_prefix + "_scaling.png";
    const std::string amdahl_png_path = output_prefix + "_amdahl.png";

    const std::vector<std::string> commands = {
        std::string("py -3 ") + quoteCommandArg(script_path),
        std::string("python3 ") + quoteCommandArg(script_path),
        std::string("python ") + quoteCommandArg(script_path)
    };

    for (const std::string& prefix : commands) {
        const std::string command = prefix +
            " --input " + quoteCommandArg(dat_path) +
            " --output " + quoteCommandArg(png_path) +
            " --amdahl-output " + quoteCommandArg(amdahl_png_path);

        if (std::system(command.c_str()) == 0) {
            return true;
        }
    }

    return false;
}

void printUsage(const char* exe) {
    std::cout
        << "Usage: " << exe << " [options]\n"
        << "  -simulate           Run simulation export mode (no realtime drawing)\n"
        << "  -steps <int>        Simulation steps for export (default: 1000)\n"
        << "  -sample <int>       Sample every K steps in trajectories.dat (default: 10)\n"
        << "  -traj-output <path> Trajectories output path (default: trajectories.dat)\n"
        << "  -energy-output <p>  Global metrics output path (default: energy_timeseries.dat)\n"
        << "  -export-energy      Also export energy/CoM/RMS time series\n"
        << "  -benchmark          Run complete suite (scaling + schedules)\n"
        << "  -scaling            Run scaling analysis\n"
        << "  -schedules          Run schedule/chunk comparison\n"
        << "  -N <int>            Problem size N (default: 1000)\n"
        << "  -iters <int>        Repetitions per experiment (default: 10)\n"
        << "  -dt <float>         Euler timestep (default: 0.01)\n"
        << "  -seed <int>         Random seed (default: 42)\n"
        << "  -threads <csv>      Thread list for scaling, e.g. 1,2,4,8\n"
        << "  -schedule <name>    static|dynamic|guided for scaling (default: static)\n"
        << "  -chunk <int>        Chunk size for scaling (default: 16)\n"
        << "  -chunks <csv>       Chunk list for schedules, e.g. 1,4,16,64\n"
        << "  -output <prefix>    Prefix for output .dat files (default: benchmark)\n"
        << "  -help               Show this help\n";
}

void printScalingSummary(const std::vector<BenchmarkRunResult>& results, bool use_full_step_time) {
    std::cout << "threads time_mean time_std speedup efficiency\n";
    for (const BenchmarkRunResult& r : results) {
        const BenchmarkStats stats = use_full_step_time ? r.full_step_time : r.compute_time;
        const double speedup = use_full_step_time ? r.full_step_speedup : r.compute_speedup;
        const double efficiency = use_full_step_time ? r.full_step_efficiency : r.compute_efficiency;

        std::cout << std::fixed << std::setprecision(6)
                  << r.threads << ' '
                  << stats.mean << ' '
                  << stats.stddev << ' '
                  << speedup << ' '
                  << efficiency << '\n';
    }
}

void printScheduleSummary(const std::vector<BenchmarkRunResult>& results, bool use_full_step_time) {
    std::cout << "schedule chunk threads time_mean time_std\n";
    for (const BenchmarkRunResult& r : results) {
        const BenchmarkStats stats = use_full_step_time ? r.full_step_time : r.compute_time;

        std::cout << std::fixed << std::setprecision(6)
                  << Benchmark::scheduleName(r.schedule_type) << ' '
                  << r.chunk_size << ' '
                  << r.threads << ' '
                  << stats.mean << ' '
                  << stats.stddev << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    constexpr double G = 1.0;
    constexpr double eps = 0.1;

    std::cout << std::fixed << std::setprecision(6);

    const std::vector<std::string> args(argv + 1, argv + argc);

    if (hasFlag(args, "-help") || hasFlag(args, "--help")) {
        printUsage(argv[0]);
        return EXIT_SUCCESS;
    }

    bool run_benchmark = hasFlag(args, "-benchmark");
    bool run_scaling = hasFlag(args, "-scaling");
    bool run_schedules = hasFlag(args, "-schedules");
    bool run_plot = hasFlag(args, "-plot");
    bool run_simulation = hasFlag(args, "-simulate");
    bool export_energy = hasFlag(args, "-export-energy");

    if (!run_benchmark && !run_scaling && !run_schedules && !run_simulation) {
        run_benchmark = true;
    }

    if (run_benchmark) {
        run_scaling = true;
        run_schedules = true;
        run_plot = true;
    }

    std::size_t n = 1000;
    int repetitions = 10;
    double dt = 0.01;
    unsigned int seed = 42;
    int sim_steps = 1000;
    int sim_sample_every = 10;

    std::vector<int> thread_counts{1, 2, 4, 8};
    int scaling_schedule = 0;
    int scaling_chunk = 16;
    std::vector<int> schedule_types{0, 1, 2};
    std::vector<int> schedule_chunks{1, 4, 16, 64};
    std::string output_prefix = "benchmark";
    std::string trajectories_output = "trajectories.dat";
    std::string energy_output = "energy_timeseries.dat";

    try {
        std::string value;
        if (getArgValue(args, "-N", value)) {
            n = static_cast<std::size_t>(std::stoull(value));
        }
        if (getArgValue(args, "-iters", value)) {
            repetitions = std::max(1, std::stoi(value));
        }
        if (getArgValue(args, "-dt", value)) {
            dt = std::stod(value);
        }
        if (getArgValue(args, "-seed", value)) {
            seed = static_cast<unsigned int>(std::stoul(value));
        }
        if (getArgValue(args, "-steps", value)) {
            sim_steps = std::max(0, std::stoi(value));
        }
        if (getArgValue(args, "-sample", value)) {
            sim_sample_every = std::max(1, std::stoi(value));
        }
        if (getArgValue(args, "-threads", value)) {
            thread_counts = parseIntCsv(value);
        }
        if (getArgValue(args, "-schedule", value)) {
            scaling_schedule = scheduleFromString(value);
        }
        if (getArgValue(args, "-chunk", value)) {
            scaling_chunk = std::max(1, std::stoi(value));
        }
        if (getArgValue(args, "-chunks", value)) {
            schedule_chunks = parseIntCsv(value);
        }
        if (getArgValue(args, "-output", value)) {
            output_prefix = value;
        }
        if (getArgValue(args, "-traj-output", value)) {
            trajectories_output = value;
        }
        if (getArgValue(args, "-energy-output", value)) {
            energy_output = value;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Invalid argument value: " << ex.what() << "\n";
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    thread_counts = sortUniquePositive(thread_counts);
    if (thread_counts.empty() || thread_counts.front() != 1) {
        thread_counts.insert(thread_counts.begin(), 1);
    }
    thread_counts = sortUniquePositive(thread_counts);
    schedule_chunks = sortUniquePositive(schedule_chunks);
    if (schedule_chunks.empty()) {
        schedule_chunks = {1, 4, 16, 64};
    }

    const std::filesystem::path results_dir = std::filesystem::current_path() / "Resultados_Benchmark";
    std::error_code fs_error;
    std::filesystem::create_directories(results_dir, fs_error);
    if (fs_error) {
        std::cerr << "Failed to create output directory: " << results_dir.string() << "\n";
        return EXIT_FAILURE;
    }

    const std::string output_base = (results_dir / output_prefix).string();

    const int schedule_threads = std::min(thread_counts.back(), maxThreads());

    if (run_simulation) {
        NBodySystem simulation_system = buildRandomSystem(n, G, eps, seed);
        SimulationExportConfig export_config;
        export_config.num_steps = sim_steps;
        export_config.sample_every = sim_sample_every;
        export_config.schedule_type = scaling_schedule;
        export_config.chunk_size = scaling_chunk;
        export_config.include_initial_state = true;
        export_config.export_global_metrics = export_energy;

        const bool ok = Visualizer::exportSimulationData(
            simulation_system,
            dt,
            trajectories_output,
            energy_output,
            export_config
        );

        if (!ok) {
            std::cerr << "Failed to export simulation data.\n";
            return EXIT_FAILURE;
        }

        std::cout << "Generated: " << trajectories_output << '\n';
        if (export_energy) {
            std::cout << "Generated: " << energy_output << '\n';
        }
    }

    std::cout << "Benchmark configuration:\n"
              << "  N           = " << n << "\n"
              << "  repetitions = " << repetitions << "\n"
              << "  dt          = " << dt << "\n"
              << "  seed        = " << seed << "\n"
              << "  schedule    = " << Benchmark::scheduleName(scaling_schedule) << "\n"
              << "  chunk       = " << scaling_chunk << "\n"
              << "  output      = " << output_base << "\n";

    if (run_scaling) {
        const std::vector<BenchmarkRunResult> scaling_results = Benchmark::runScalingAnalysis(
            n,
            thread_counts,
            repetitions,
            dt,
            G,
            eps,
            scaling_schedule,
            scaling_chunk,
            seed
        );

        const std::string scaling_compute_path = output_base + "_scaling.dat";
        const std::string scaling_full_step_path = output_base + "_scaling_fullstep.dat";

        Benchmark::exportScalingDat(scaling_compute_path, scaling_results, false);
        Benchmark::exportScalingDat(scaling_full_step_path, scaling_results, true);

        std::cout << "\nScaling analysis (computeAccelerations):\n";
        printScalingSummary(scaling_results, false);
        std::cout << "\nScaling analysis (full Euler step):\n";
        printScalingSummary(scaling_results, true);
        std::cout << "\nGenerated: " << scaling_compute_path << '\n';
        std::cout << "Generated: " << scaling_full_step_path << '\n';

        if (run_plot) {
            const std::string script_path = "plot_scaling.py";
            const bool plotted = runPlotScript(script_path, scaling_compute_path, output_base);
            std::cout << "Plot script: " << (plotted ? "OK" : "ERROR") << '\n';
        }
    }

    if (run_schedules) {
        const std::vector<BenchmarkRunResult> schedule_results = Benchmark::runScheduleComparison(
            n,
            schedule_threads,
            repetitions,
            dt,
            G,
            eps,
            schedule_types,
            schedule_chunks,
            seed
        );

        const std::string schedules_compute_path = output_base + "_schedules.dat";
        const std::string schedules_full_step_path = output_base + "_schedules_fullstep.dat";

        Benchmark::exportScheduleDat(schedules_compute_path, schedule_results, false);
        Benchmark::exportScheduleDat(schedules_full_step_path, schedule_results, true);

        std::cout << "\nSchedule comparison (computeAccelerations):\n";
        printScheduleSummary(schedule_results, false);
        std::cout << "\nSchedule comparison (full Euler step):\n";
        printScheduleSummary(schedule_results, true);
        std::cout << "\nGenerated: " << schedules_compute_path << '\n';
        std::cout << "Generated: " << schedules_full_step_path << '\n';
    }

    return EXIT_SUCCESS;
}