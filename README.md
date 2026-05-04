# N-Body 2D con OpenMP

Simulador 2D de N-cuerpos en C++17 con soporte OpenMP, integración Euler y un módulo de benchmarks para análisis de scaling, comparación de schedules, eficiencia y Ley de Amdahl. El proyecto cuenta con un pipeline de Integración Continua (CI) completamente dockerizado.

## Descripción del proyecto

El proyecto está organizado en `nbody_2d/` y contiene:

- `Particle`: partículas con masa, posición, velocidad y aceleración.
- `NBodySystem`: cálculo de aceleraciones serial y paralelo con OpenMP.
- `NBodySimulator`: integración Euler sobre el sistema.
- `MetricsCalculator`: métricas físicas básicas.
- `Benchmark`: medición de tiempo, estadísticas, speedup, eficiencia, Amdahl y exportación a `.dat`.
- `plot_reports.py`: script externo en Python para generar gráficos PNG.

## Ejecución con Docker

Usando GitHub Container Registry (GHCR) no será necesario que instales dependencias locales (C++, Python, Matplotlib, Catch2) para ejecutar. Todo está empaquetado en una imagen base.

**Requisito único:** Tener [Docker](https://docs.docker.com/get-docker/) instalado.

### 1. Descargar la imagen base
```bash
docker pull ghcr.io/nicom04/lab1-distri:base
```

### 2. Ejección usando el contenedor
Estando en la carpeta `/nbody_2d`, utiliza los siguientes comandos. Los volúmenes (-v) aseguran que los gráficos generados se muestre en la máquina local.

**Generar los Benchmarks y las Gráficos de Rendimiento**:
```bash
docker run --rm -v $(pwd):/workspace -w /workspace ghcr.io/nicom04/lab1-distri:base bash -c "make benchmark"
```

**Generar el Análisis Físico y las Trayectorias**:
```bash
docker run --rm -v $(pwd):/workspace -w /workspace ghcr.io/nicom04/lab1-distri:base bash -c "make analysis"
```

**Ejecutar los Tests Unitarios**:
```bash
docker run --rm -v $(pwd):/workspace -w /workspace ghcr.io/nicom04/lab1-distri:base bash -c "make clean && make all && make test"
```

Los archivos generados aparecerán directamente en tu carpeta de Windows/WSL. Busca la carpeta `Resultados_Benchmark` y `Resultados_Analisis` localmente.

## Integración Continua (CI/CD)

El proyecto utiliza **GitHub Actions** :

1. **Toolchain Image** (`build-base-container`): Construye y publica automáticamente una imagen Docker con las herramientas necesarias a GHCR cuando se modifica el `Dockerfile`.

2. **CI Flow** (`ci.yml`): En cada `push` o `Pull Request` al código fuente, se inyecta el código en la imagen base y se ejecuta `make test` en un entorno limpio para prevenir regresiones.



## Requisitos previos para ejecución Local

Para compilar y ejecutar el proyecto necesitas:

- **Compilador C++17**: g++ o clang con soporte OpenMP.
- **Catch2 (v3)**: Para las pruebas unitarias.
- **Python 3** (opcional, solo para generar gráficos): `python3`.
- **Matplotlib** (opcional): `pip install matplotlib`.

```bash
# Verificar que tienes g++ y OpenMP
g++ --version
g++ -fopenmp -c test.cpp  # Verifica OpenMP

# Instalar Python y matplotlib (si no tienes)
apt-get install python3 python3-pip  # Linux
pip install matplotlib
```

## Compilación e instalación rápida

Desde la carpeta `nbody_2d/`:

```bash
make                 # Compila el ejecutable nbody_2d
make benchmark       # Ejecuta todos los benchmarks y genera Resultados_Benchmark/
make analysis        # Ejecuta la simulación y genera Resultados_Analisis/
make test            # Compila y ejecuta los tests unitarios con Catch2
make clean           # Limpia archivos compilados
```

## Explicación de los comandos Make

### `make benchmark` (Benchmarks / rendimiento)

Ejecuta el conjunto completo de benchmarks y genera gráficos de rendimiento.

```bash
make benchmark
```

**Qué hace:**
1. **Benchmark completo** (`-benchmark`): Mide el tiempo base del simulador con configuración predeterminada
2. **Análisis de scaling** (`-scaling`): Prueba con 1, 2, 4 y 8 hilos para medir speedup y eficiencia
3. **Comparación de schedules** (`-schedules`): Compara diferentes estrategias de distribución de trabajo (static, dynamic, guided) con diferentes tamaños de chunk
4. **Generación de gráficos** (`-plot`): Ejecuta el script Python para generar visualizaciones

**Salidas generadas en `Resultados_Benchmark/`:**
- `benchmark_full_scaling.dat`: Datos de tiempo y speedup
- `benchmark_full_schedules.dat`: Datos de comparación de schedules
- `performance_plots.png`: Gráfico visualizando speedup, eficiencia y Ley de Amdahl
- Archivos `.dat` adicionales con variantes de mediciones

---

### `make analysis` (Análisis físico)

Ejecuta la simulación de N-cuerpos y genera gráficos del análisis físico.

```bash
make analysis
```

**Qué hace:**
1. **Simulación** (`-simulate`): Ejecuta 1000 pasos de integración Euler, muestreando posiciones cada 10 pasos
2. **Cálculo de energía**: Exporta la serie temporal de energía total del sistema
3. **Generación de gráficos**: Crea visualización de trayectorias y evolución de energía

**Salidas generadas en `Resultados_Analisis/`:**
- `trajectories.dat`: Posiciones (x, y) de las 12 primeras partículas en cada paso muestreado
- `energy_timeseries.dat`: Energía cinética, potencial y total en cada paso
- `physics_plots.png`: Gráfico de trayectorias 2D y evolución de energía

---

### `make test` (Tests unitarios)

Compila y ejecuta los tests unitarios del proyecto usando Catch2.

```bash
make test
```

**Qué hace:**
- Compila todos los tests en `tests/` junto con la lógica principal
- Ejecuta pruebas para validar:
  - Cálculo correcto de aceleraciones
  - Cumplimiento de Tercera Ley de Newton
  - Integración numérica correcta
  - Casos especiales (aceleraciones nulas, etc.)
  - Regresiones del código

**Salida:** Reporte detallado de tests que pasaron o fallaron

---

## Flujo típico de uso

```bash
# 1. Compilar
make

# 2. Ejecutar tests (opcional, para validar)
make test

# 3. Ejecutar benchmarks de rendimiento
make benchmark

# 4. Ejecutar análisis físico
make analysis

# 5. Ver resultados
# Resultados_Benchmark/ contiene gráficos de performance
# Resultados_Analisis/ contiene gráficos físicos

# 6. Limpiar (opcional, cuando termines)
make clean
```

## Ejecución manual de benchmarks

Si prefieres ejecutar benchmarks individuales manualmente:

### Benchmark completo

```bash
./nbody_2d -benchmark
```

### Scaling

```bash
./nbody_2d -scaling -N 4000 -iters 10 -threads 1,2,4,8 -schedule static -chunk 16 -output wk4
```

### Comparación de schedules

```bash
./nbody_2d -schedules -N 4000 -iters 10 -threads 1,2,4,8 -chunks 1,4,16,64 -output wk4
```

### Generación automática de gráficos

```bash
./nbody_2d -scaling -N 4000 -iters 10 -threads 1,2,4,8 -schedule static -chunk 16 -output wk4 -plot
```

## Parámetros

- `-N`: número de partículas del problema
- `-iters`: repeticiones por experimento para calcular media y desviación estándar
- `-threads`: lista separada por comas con los hilos a probar, por ejemplo `1,2,4,8`
- `-schedule`: tipo de schedule OpenMP para `computeAccelerations`
- `-chunk`: chunk size usado por `omp_set_schedule`
- `-output`: prefijo de salida para archivos `.dat` y `.png`
- `-plot`: ejecuta automáticamente el script Python de graficación
- `-simulate`: ejecuta simulación física
- `-steps`: número de pasos de integración
- `-sample`: frecuencia de muestreo de posiciones
- `-traj-output`: archivo de salida para trayectorias
- `-export-energy`: exporta series de energía
- `-energy-output`: archivo de salida para energía

## Salidas generadas

### Resultados_Benchmark/
- `benchmark_full_scaling.dat`: datos de tiempo vs threads
- `benchmark_full_schedules.dat`: datos de schedules
- `performance_plots.png`: gráfico de speedup y eficiencia

### Resultados_Analisis/
- `trajectories.dat`: posiciones de partículas
- `energy_timeseries.dat`: energía total en cada paso
- `physics_plots.png`: gráfico de trayectorias y energía

## Gráficos

### performance_plots.png (Resultados_Benchmark/)

El script `plot_reports.py` genera:
- Tiempo promedio vs threads con barras de error
- Speedup vs threads
- Eficiencia vs threads
- Comparación entre speedup medido y speedup teórico de Amdahl

### physics_plots.png (Resultados_Analisis/)

- Trayectorias 2D de las primeras partículas
- Evolución temporal de energía cinética, potencial y total

## Speedup y eficiencia

- Speedup: $S_p = T_1 / T_p$
- Eficiencia: $E_p = S_p / p$

Donde:
- $T_1$ es el tiempo con un hilo
- $T_p$ es el tiempo con $p$ hilos

## Ley de Amdahl

El benchmark estima la fracción serial $f$ a partir de resultados medidos y calcula:

$$S_p = \frac{1}{f + \frac{1-f}{p}}$$

Ese valor teórico se exporta junto a los datos medidos para comparar escalabilidad real vs esperada.

## Notas

- El proyecto conserva compatibilidad con los comandos existentes
- La lógica física del simulador no se modifica
- Los gráficos se generan a partir de los `.dat` ya exportados
- Los resultados se organizan en carpetas separadas: `Resultados_Benchmark/` y `Resultados_Analisis/`

