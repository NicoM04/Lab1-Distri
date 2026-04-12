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

- WSL Ubuntu con g++ (>= 11) y make instalados

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

5. Ejecutar test básico de aceleración:

```bash
make test
```

## Docker

```bash
docker build -t nbody_2d .
docker run --rm nbody_2d
```