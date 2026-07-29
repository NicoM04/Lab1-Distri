# nbody_2d

Implementacion inicial serial para el Laboratorio 1 de N-Cuerpos.

Estado actual:
- Semana 1 completa.
- Semana 2 completa: computeAccelerations en paralelo (OpenMP), comparacion serial vs paralelo para N pequeno y verificacion con tolerancia de coma flotante.

## Estructura

- main.cpp
- Particle.h / Particle.cpp
- NBodySystem.h / NBodySystem.cpp
- NBodySimulator.h / NBodySimulator.cpp
- Integrator.h / Integrator.cpp
- MetricsCalculator.h / MetricsCalculator.cpp
- Benchmark.h / Benchmark.cpp
- Visualizer.h / Visualizer.cpp
- tests/
- Dockerfile
- Makefile

## Memoria CUDA y layout SoA

La capa CUDA del laboratorio usa un layout SoA (Structure of Arrays) para mantener cada atributo físico en un buffer device separado administrado por `CudaBuffer<T>`:


Esto favorece coalescing porque hilos consecutivos leen elementos consecutivos del mismo arreglo, por ejemplo `d_x[i]`, `d_x[i+1]`, `d_x[i+2]`, en vez de saltar entre campos mezclados de un AoS.

La memoria device se administra con `CudaBuffer<T>`, una clase RAII que reserva con `cudaMalloc` y libera con `cudaFree` automáticamente. La copia de estado sigue este ciclo:

1. Al construir o reacomodar el sistema, se reserva memoria device para todos los buffers SoA.
2. Antes del kernel de aceleraciones, se suben a device `mass`, `x`, `y`, `vx` y `vy` solo si el estado host cambió.
3. Tras el kernel, se llama a `cudaDeviceSynchronize()` mediante `NBodySystem::synchronizeDevice()`.
4. Luego se bajan `ax` y `ay` al host para que Euler actualice las partículas.
5. Después de Euler, si el siguiente paso vuelve a usar GPU, se re-sube solo el estado host actualizado necesario para el siguiente kernel.

La clase `NBodySystem` expone dos rutas claras para la parte CUDA:

- `computeAccelerationsGpuKernelOnly(...)`, que deja el kernel lanzado pero no fuerza la descarga de resultados.
- `computeAccelerationsGpu(...)`, que ejecuta la ruta completa kernel + sincronización + descarga de `ax/ay`.

La clase también expone `deviceTransferCount()` para dejar trazable cuántas copias reales hace el pipeline por paso.

Transferencias por paso temporal en la ruta híbrida actual:

- Inicialización: 1 copia H2D de `mass` y 4 copias H2D de estado `x/y/vx/vy`.
- Cada paso con GPU: 1 sincronización, 2 copias D2H de aceleraciones `ax/ay`.
- Cada paso con Euler en host y luego otro kernel GPU: 4 copias H2D de estado actualizado `x/y/vx/vy`.

El contador `deviceTransferCount()` permite verificar este número de transferencias en pruebas o logs.

## Tolerancia numérica

Las transferencias de memoria no cambian la tolerancia física del modelo. Las comparaciones numéricas siguen usando las tolerancias ya definidas en los tests de aceleración e integración; la única precaución es mantener la misma precisión de tipo entre host y device para no introducir diferencias adicionales.

## Requisitos

- WSL Ubuntu con g++ (>= 11) y make instalados
- Python 3 con matplotlib para generar PNG (`python3 -m pip install --user matplotlib`)

## Compilar y ejecutar

1. Abrí una terminal WSL Ubuntu en VS Code.
2. Navegá al directorio del proyecto:

```bash
cd "/mnt/c/Users/matia/OneDrive/Escritorio/Lab1 Distri/nbody_2d"
```

3. Limpiar y compilar:

```bash
make clean
make
```

4. Ejecutar simulación base:

```bash
./nbody_2d
```

5. Ejecutar benchmark de rendimiento:

```bash
make benchmark
```

Este comando genera:
- `Resultados_Benchmark/benchmark_scaling.dat` y `Resultados_Benchmark/benchmark_schedules.dat`.
- `performance_plots.png` con speedup, eficiencia, comparación chunk/schedule y curva de Amdahl.

6. Ejecutar análisis físico (usa Visualizer para exportar datos):

```bash
make analysis
```

También podés usar el alias pedido:

```bash
make analisys
```

Este comando genera:
- `trajectories.dat` con posiciones muestreadas (step, time, id, x, y).
- `energy_timeseries.dat` con energía total y métricas globales (si está activado en el target).
- `physics_plots.png` con trayectorias de un subconjunto y deriva de energía total.

7. Ejecutar test básico de aceleración:

```bash
make test
```

Esto ejecuta:
- `tests/test_acceleration` (caso base semana 1).
- `tests/test_parallel_vs_serial` (semana 2: equivalencia serial/paralelo con tolerancia `1e-12`).

## Semana 2: Paralelizacion y comparacion

- `NBodySystem::computeAccelerationsParallel(schedule_type, chunk_size)` usa OpenMP.
- `schedule_type`: `0=static`, `1=dynamic`, `2=guided`, `3=auto`.
- `Benchmark::compareSerialVsParallel(...)` reporta tiempo serial/paralelo, speedup y diferencia maxima de aceleracion.
- En `main.cpp` se imprime la comparacion con `N` pequeno y se valida `max |dA| <= 1e-12`.

## Docker

```bash
docker build -t nbody_2d .
docker run --rm nbody_2d
```