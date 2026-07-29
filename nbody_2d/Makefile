CXX = g++
CUDA_HOME ?= /usr/local/cuda
NVCC = $(CUDA_HOME)/bin/nvcc

CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -fopenmp -I. -Itests -I$(CUDA_HOME)/include
# Bandera de enlazado para la API de CUDA
LDFLAGS = -L$(CUDA_HOME)/lib64 -lcudart

# Arquitectura CUDA configurable.
# GTX 1650: compute capability 7.5.
CUDA_ARCH ?= 75
NVCCFLAGS = -O3 -std=c++17 -arch=sm_$(CUDA_ARCH) -Xcompiler=-Wall,-Wextra -I. -Itests -I$(CUDA_HOME)/include

# Librerías de Catch2 v3
TEST_LIBS = -lCatch2Main -lCatch2

TARGET = nbody_2d
TEST_TARGET = run_tests
CUDA_TEST_TARGET = gpu_accelerations_smoke_test
CUDA_BUFFER_TEST_TARGET = cuda_buffer_roundtrip_test

# --- ARCHIVOS FUENTE ---

MAIN_SRC = main.cpp

CORE_SRCS = Particle.cpp \
            NBodySystem.cpp \
            NBodySimulator.cpp \
            Integrator.cpp \
            MetricsCalculator.cpp \
            Benchmark.cpp \
            Visualizer.cpp

# Fuente CUDA que debe incluirse en el ejecutable principal
CU_CORE_SRCS = kernels/accelerations.cu

CORE_OBJS = $(CORE_SRCS:.cpp=.o)
CU_CORE_OBJS = $(CU_CORE_SRCS:.cu=.o)

APP_OBJS = main.o $(CORE_OBJS) $(CU_CORE_OBJS)

# Tests CPU
TEST_SRCS = tests/test_acceleration.cpp \
            tests/test_parallel_vs_serial.cpp \
            tests/test_integration.cpp \
            tests/test_action_reaction.cpp \
            tests/test_zero_accelerations.cpp \
            tests/test_regression.cpp

TEST_OBJS = $(TEST_SRCS:.cpp=.o)

# Test CUDA del Rol 1
CUDA_TEST_SRCS = tests/gpu_accelerations_smoke_test.cu \
                 kernels/accelerations.cu

CUDA_BUFFER_TEST_SRCS = tests/cuda_buffer_roundtrip_test.cu

.PHONY: all clean run test cuda-build cuda-test test-all benchmark analysis

all: $(TARGET)

# Regla general para archivos C++
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regla general para compilar objetos CUDA
%.o: %.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# --- APLICACIÓN PRINCIPAL ---

$(TARGET): $(APP_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

# --- BENCHMARK ---

benchmark: $(TARGET)
	mkdir -p Resultados_Benchmark
	./$(TARGET) -benchmark -output benchmark_full
	python3 plot_reports.py \
		--scaling Resultados_Benchmark/benchmark_full_scaling.dat \
		--schedules Resultados_Benchmark/benchmark_full_schedules.dat \
		--performance-output Resultados_Benchmark/performance_plots.png

# --- ANÁLISIS ---

analysis: $(TARGET)
	mkdir -p Resultados_Analisis
	./$(TARGET) -simulate \
		-N 1000 \
		-iters 10 \
		-dt 0.01 \
		-seed 42 \
		-steps 1000 \
		-sample 10 \
		-traj-output Resultados_Analisis/trajectories.dat \
		-export-energy \
		-energy-output Resultados_Analisis/energy_timeseries.dat
	python3 plot_reports.py \
		--trajectories Resultados_Analisis/trajectories.dat \
		--energy Resultados_Analisis/energy_timeseries.dat \
		--physics-output Resultados_Analisis/physics_plots.png \
		--subset-bodies 12

# --- TESTS CPU CON CATCH2 ---

$(TEST_TARGET): $(TEST_OBJS) $(CORE_OBJS) $(CU_CORE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(TEST_LIBS) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

# --- TESTS CUDA ---

$(CUDA_TEST_TARGET): $(CUDA_TEST_SRCS) \
                     CudaCheck.cuh \
                     kernels/accelerations.cuh
	$(NVCC) $(NVCCFLAGS) $(CUDA_TEST_SRCS) -o $@

$(CUDA_BUFFER_TEST_TARGET): $(CUDA_BUFFER_TEST_SRCS) \
                            CudaBuffer.h \
                            CudaCheck.cuh
	$(NVCC) $(NVCCFLAGS) $(CUDA_BUFFER_TEST_SRCS) -o $@

cuda-build: $(CUDA_TEST_TARGET) $(CUDA_BUFFER_TEST_TARGET)

cuda-test: $(CUDA_TEST_TARGET) $(CUDA_BUFFER_TEST_TARGET)
	./$(CUDA_TEST_TARGET)
	./$(CUDA_BUFFER_TEST_TARGET)

# Ejecuta primero los tests CPU y luego los tests CUDA.
test-all: test cuda-test

# --- LIMPIEZA ---

clean:
	rm -f $(APP_OBJS) \
	      $(TEST_OBJS) \
	      $(TARGET) \
	      $(TEST_TARGET) \
	      $(CUDA_TEST_TARGET) \
	      $(CUDA_BUFFER_TEST_TARGET) \
	      kernels/*.o