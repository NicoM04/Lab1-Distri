# N-Body 2D con OpenMP

Simulador 2D de N-cuerpos en C++17 con soporte OpenMP, integracion Euler y un modulo de benchmarks para analisis de scaling, comparacion de schedules, eficiencia y Ley de Amdahl.

## Descripcion del proyecto

El proyecto esta organizado en `nbody_2d/` y contiene:

- `Particle`: particulas con masa, posicion, velocidad y aceleracion.
- `NBodySystem`: calculo de aceleraciones serial y paralelo con OpenMP.
- `NBodySimulator`: integracion Euler sobre el sistema.
- `MetricsCalculator`: metricas fisicas basicas.
- `Benchmark`: medicion de tiempo, estadisticas, speedup, eficiencia, Amdahl y exportacion a `.dat`.
- `plot_scaling.py`: script externo en Python para generar graficos PNG.

## Compilacion

Desde la carpeta `nbody_2d/`:

```bash
make clean
make
```

El ejecutable resultante es `nbody_2d`.

## Ejecucion de benchmarks

### Benchmark completo

```bash
./nbody_2d -benchmark
```

### Scaling

```bash
./nbody_2d -scaling -N 4000 -iters 10 -threads 1,2,4,8 -schedule static -chunk 16 -output wk4
```

### Comparacion de schedules

```bash
./nbody_2d -schedules -N 4000 -iters 10 -threads 1,2,4,8 -chunks 1,4,16,64 -output wk4
```

### Generacion automatica de graficos

```bash
./nbody_2d -scaling -N 4000 -iters 10 -threads 1,2,4,8 -schedule static -chunk 16 -output wk4 -plot
```

## Parametros

- `-N`: numero de particulas del problema.
- `-iters`: repeticiones por experimento para calcular media y desviacion estandar.
- `-threads`: lista separada por comas con los hilos a probar, por ejemplo `1,2,4,8`.
- `-schedule`: tipo de schedule OpenMP para `computeAccelerations`.
- `-chunk`: chunk size usado por `omp_set_schedule`.
- `-output`: prefijo de salida para archivos `.dat` y `.png` (se guardan en `../Resultados`).
- `-plot`: ejecuta automaticamente el script Python de graficacion despues de generar los `.dat`.

## Salidas generadas

Con `-output wk4` se generan archivos en la carpeta `Resultados` (al mismo nivel que `nbody_2d`), por ejemplo:

- `Resultados/wk4_scaling.dat`
- `Resultados/wk4_scaling_fullstep.dat`
- `Resultados/wk4_schedules.dat`
- `Resultados/wk4_schedules_fullstep.dat`
- `Resultados/wk4_scaling.png`
- `Resultados/wk4_amdahl.png`

### Formato de `wk4_scaling.dat`

```text
threads time_mean time_std speedup efficiency amdahl_speedup
1 10.0 0.2 1.0 1.0 1.0
2 5.5 0.1 1.8 0.9 1.7
```

## Graficos

El script `plot_scaling.py` genera:

- tiempo promedio vs threads con barras de error
- speedup vs threads
- eficiencia vs threads
- comparacion entre speedup medido y speedup teorico de Amdahl

## Speedup y eficiencia

- Speedup: $S_p = T_1 / T_p$
- Eficiencia: $E_p = S_p / p$

Donde:
- $T_1$ es el tiempo con un hilo.
- $T_p$ es el tiempo con $p$ hilos.

## Ley de Amdahl

El benchmark estima la fraccion serial $f$ a partir de resultados medidos y calcula:

$$
S_p = \frac{1}{f + \frac{1-f}{p}}
$$

Ese valor teorico se exporta junto a los datos medidos para comparar escalabilidad real vs esperada.

## Requisitos del script de graficos

Para usar `-plot` necesitas Python y matplotlib:

```bash
pip install matplotlib
```

Si el sistema usa otro comando de Python, el ejecutable intenta `py -3` y luego `python`.

## Notas

- El proyecto conserva compatibilidad con los comandos existentes.
- La logica fisica del simulador no se modifica.
- Los graficos se generan a partir de los `.dat` ya exportados, no desde C++.
