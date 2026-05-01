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