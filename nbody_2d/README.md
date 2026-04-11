# nbody_2d

Implementacion inicial serial para el Laboratorio 1 de N-Cuerpos.

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

- Compilador compatible con C++17
- make

## Compilar y ejecutar

```bash
make
./nbody_2d
```

## Docker

```bash
docker build -t nbody_2d .
docker run --rm nbody_2d
```