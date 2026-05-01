CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -fopenmp

# Librerías de Catch2 v3
TEST_LIBS = -lCatch2Main -lCatch2

TARGET = nbody_2d
TEST_TARGET = run_tests

# --- ARCHIVOS FUENTE ---
# Separamos el main.cpp de la lógica principal, para no tener conflictos en los tests.
MAIN_SRC = main.cpp
CORE_SRCS = Particle.cpp NBodySystem.cpp NBodySimulator.cpp Integrator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp
CORE_OBJS = $(CORE_SRCS:.cpp=.o)

APP_OBJS = main.o $(CORE_OBJS)

# Tests
TEST_SRCS = tests/test_acceleration.cpp \
            tests/test_parallel_vs_serial.cpp \
            tests/test_integration.cpp \
            tests/test_action_reaction.cpp \
            tests/test_zero_accelerations.cpp \
            tests/test_regression.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

.PHONY: all clean run test benchmark analysis analisys

all: $(TARGET)

# Regla general
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- APLICACIÓN PRINCIPAL ---
$(TARGET): $(APP_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

# --- BENCHMARK ---
# Ejecuta: full, scaling, schedules, plotting y genera performance_plots.png
benchmark: $(TARGET)
	mkdir -p Resultados_Benchmark
# 1) Benchmark completo
	./$(TARGET) -benchmark -output benchmark_full
# 2) Scaling
	./$(TARGET) -scaling -N 4000 -iters 10 -threads 1,2,4,8 -schedule static -chunk 16 -output wk4_scaling
# 3) Comparación de schedules
	./$(TARGET) -schedules -N 4000 -iters 10 -threads 1,2,4,8 -chunks 1,4,16,64 -output wk4_schedules
# 4) Generación automática de gráficos (plot)
	./$(TARGET) -scaling -N 4000 -iters 10 -threads 1,2,4,8 -schedule static -chunk 16 -output wk4 -plot
# 5) Generar performance_plots.png
	python3 plot_reports.py --scaling Resultados_Benchmark/benchmark_full_scaling.dat --schedules Resultados_Benchmark/benchmark_full_schedules.dat --performance-output Resultados_Benchmark/performance_plots.png

# --- ANALYSIS ---
# Genera trayectorias/energía y physics_plots.png
analysis: $(TARGET)
	mkdir -p Resultados_Analisis
# Genera trayectorias y energía
	./$(TARGET) -simulate -steps 1000 -sample 10 -traj-output Resultados_Analisis/trajectories.dat -export-energy -energy-output Resultados_Analisis/energy_timeseries.dat
# Generar physics_plots.png
	python3 plot_reports.py --trajectories Resultados_Analisis/trajectories.dat --energy Resultados_Analisis/energy_timeseries.dat --physics-output Resultados_Analisis/physics_plots.png --subset-bodies 12

# --- CATCH2 (para los tests)---
# Compila todo para un ejecutable para los test (sin el main.o)
$(TEST_TARGET): $(TEST_OBJS) $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(TEST_LIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Limpieza
clean:
	rm -f $(APP_OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)