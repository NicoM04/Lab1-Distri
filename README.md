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

## Roles del equipo

| Rol | Encargado |
|---|---|
| Modelo y Datos | Nicolás Morales |
| Núcleo paralelo | Gabriel Cabrera |
| Integración y Física | Amaru Monje |
| Métricas y Benchmark | Francisco Riquelme |
| Calidad, CI y visualización | Thomas Gustafsson |

## Aporte CUDA del Rol 2

La memoria device del simulador 2D CUDA se organiza en SoA para favorecer coalescing:

- `d_mass`
- `d_x`, `d_y`
- `d_vx`, `d_vy`
- `d_ax`, `d_ay`

Cada arreglo vive dentro de `CudaBuffer<T>`, una clase RAII que reserva con `cudaMalloc` y libera con `cudaFree` automáticamente. El ciclo de ejecución queda así:

1. Se reserva memoria device al inicializar el sistema.
2. Se suben a device las masas y el estado de partículas solo cuando el host cambia.
3. Se ejecuta el kernel de aceleraciones.
4. Se sincroniza con `cudaDeviceSynchronize()` a través de `NBodySystem::synchronizeDevice()`.
5. Se bajan `ax/ay` al host para el Euler en CPU.
6. Se re-suben solo `x/y/vx/vy` cuando el siguiente paso vuelve a necesitar GPU.

Con este diseño, el número de transferencias por paso temporal se reduce al mínimo práctico: dos copias D2H de aceleraciones y, solo si el estado cambió en host, cuatro copias H2D del estado de partículas.

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
1. **Benchmark completo** (`-benchmark`): Mide el tiempo base del simulador con configuración predeterminada.
2. **Análisis de scaling** (`-scaling`): Prueba con 1, 2, 4 y 8 hilos para medir speedup y eficiencia.
3. **Comparación de schedules** (`-schedules`): Compara diferentes estrategias de distribución de trabajo (static, dynamic, guided) con diferentes tamaños de chunk.
4. **Generación de gráficos** (`-plot`): Ejecuta el script Python para generar visualizaciones.

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

**Parámetros por defecto**

Se toma la "misma" configuración que para el `make Benchmark`.
- N = 1000
- iters = 10
- dt = 0.01
- seed = 42
- steps = 1000
 -sample = 10
- subset-bodies = 12
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



## Implementación CUDA — Laboratorio 2

### Kernels de aceleraciones

Se implementaron dos variantes CUDA para calcular las aceleraciones gravitatorias del sistema de N cuerpos:

- `variant = 0`: kernel básico.
- `variant = 1`: kernel con memoria compartida.

En ambas variantes se asigna un hilo CUDA a cada cuerpo `i`. El índice global del cuerpo se calcula mediante:

```cpp
i = blockIdx.x * blockDim.x + threadIdx.x;
```

La cantidad de bloques de la grilla se obtiene utilizando división a techo:

```cpp
gridSize = (N + blockSize - 1) / blockSize;
```

Esto permite procesar correctamente cantidades de cuerpos que no sean múltiplos del tamaño del bloque.

---

### Kernel básico

El kernel `computeAccelerationsKernel` asigna un cuerpo `i` a cada hilo CUDA.

Cada hilo:

1. Calcula su índice global.
2. Verifica que `i < N`.
3. Lee la posición del cuerpo `i`.
4. Recorre secuencialmente todos los cuerpos `j`.
5. Omite la interacción cuando `j == i`.
6. Acumula las componentes de aceleración `ax` y `ay`.
7. Escribe el resultado en:

```text
d_ax[i]
d_ay[i]
```

Cada hilo escribe únicamente la aceleración correspondiente a su propio cuerpo. Por esta razón, no se requieren operaciones atómicas en los kernels de aceleraciones.

La protección de bordes se realiza mediante:

```cpp
if (i >= n) {
    return;
}
```

---

### Kernel con memoria compartida

El kernel `computeAccelerationsKernelShared` utiliza memoria compartida para reducir accesos repetidos a la memoria global.

Los cuerpos se procesan en grupos o *tiles*. Para cada tile, los hilos del bloque cargan temporalmente:

```text
sharedMass
sharedX
sharedY
```

La memoria compartida dinámica se distribuye de la siguiente forma:

```text
sharedMass[blockDim.x]
sharedX[blockDim.x]
sharedY[blockDim.x]
```

Cada hilo carga como máximo un cuerpo del tile. Cuando el último tile contiene menos cuerpos que `blockDim.x`, los elementos restantes se inicializan en cero para evitar accesos fuera de rango.

Se utiliza `__syncthreads()` en dos momentos:

1. Después de cargar los datos del tile.
2. Después de utilizar el tile y antes de reemplazarlo por el siguiente.

Los hilos cuyo índice cumple `i >= N` no calculan ni escriben aceleraciones, pero deben participar en las llamadas a `__syncthreads()`.

Por esta razón, la variante shared utiliza una variable lógica:

```cpp
const bool active = i < n;
```

en lugar de retornar inmediatamente cuando `i >= N`.

Esto evita que algunos hilos abandonen el kernel antes de una barrera de sincronización.

---

### Layout de memoria

Los kernels utilizan un layout `Structure of Arrays` o SoA en memoria device:

```text
d_mass
d_x
d_y
d_ax
d_ay
```

Cada arreglo almacena una sola propiedad para todos los cuerpos:

```text
d_mass = [mass0, mass1, mass2, ...]
d_x    = [x0, x1, x2, ...]
d_y    = [y0, y1, y2, ...]
d_ax   = [ax0, ax1, ax2, ...]
d_ay   = [ay0, ay1, ay2, ...]
```

Este diseño permite que los hilos consecutivos accedan a posiciones consecutivas de memoria, favoreciendo accesos coalescentes en memoria global.

El almacenamiento CPU original mediante `std::vector<Particle>` se conserva como referencia de corrección.

---

### Interfaz de lanzamiento

El lanzador general de los kernels utiliza la siguiente interfaz:

```cpp
launchComputeAccelerations(
    d_mass,
    d_x,
    d_y,
    d_ax,
    d_ay,
    n,
    gravitationalConstant,
    epsilon,
    variant,
    blockSize
);
```

Las variantes disponibles son:

```text
0: kernel básico
1: kernel con memoria compartida
```

También existen lanzadores específicos:

```cpp
launchComputeAccelerationsBasic(...);
launchComputeAccelerationsShared(...);
```

El tamaño del bloque se recibe como parámetro, lo que permite probar diferentes valores de `blockDim.x`.

---

### Contrato con la capa host/device

Los lanzadores CUDA implementados por el Rol 1 cumplen el siguiente contrato:

- No reservan memoria mediante `cudaMalloc`.
- No liberan memoria mediante `cudaFree`.
- No realizan transferencias H2D.
- No realizan transferencias D2H.
- No llaman internamente a `cudaDeviceSynchronize()`.
- Reciben punteros previamente asignados en memoria device.
- Calculan automáticamente la cantidad de bloques mediante división a techo.
- Comprueban errores inmediatos de lanzamiento con `cudaGetLastError()`.
- Permiten seleccionar la variante y el tamaño del bloque.

La capa host/device desarrollada por el Rol 2 será responsable de:

1. Crear los buffers device.
2. Transformar los datos CPU desde AoS hacia SoA.
3. Realizar las transferencias H2D.
4. Invocar el kernel correspondiente.
5. Sincronizar el device.
6. Realizar las transferencias D2H.
7. Actualizar las aceleraciones almacenadas en los objetos `Particle`.
8. Liberar los buffers mediante una solución RAII.

---

### Manejo de errores CUDA

Se implementó la macro:

```cpp
CUDA_CHECK(call)
```

Esta macro comprueba el valor retornado por las funciones de la API CUDA y muestra:

- Descripción del error.
- Expresión ejecutada.
- Archivo donde ocurrió.
- Número de línea.

Ejemplos de uso:

```cpp
CUDA_CHECK(cudaMalloc(&devicePointer, bytes));
CUDA_CHECK(cudaMemcpy(destination, source, bytes, cudaMemcpyHostToDevice));
CUDA_CHECK(cudaDeviceSynchronize());
CUDA_CHECK(cudaFree(devicePointer));
```

Después de lanzar cada kernel se ejecuta:

```cpp
CUDA_CHECK(cudaGetLastError());
```

Esto permite detectar errores inmediatos en la configuración o lanzamiento del kernel.

La sincronización se realiza desde el test, simulador o benchmark, y no desde el lanzador.

---

### Validación CPU vs. GPU

La versión CPU serial del Laboratorio 1 se conserva como referencia de corrección.

Las comparaciones de aceleraciones utilizan las siguientes tolerancias:

```text
rtol = 1e-4
atol = 1e-8
```

El criterio de comparación utilizado es:

```text
|resultado_gpu - resultado_cpu|
<= atol + rtol × |resultado_cpu|
```

Se realizan las siguientes comparaciones:

- CPU serial vs. kernel básico.
- CPU serial vs. kernel shared.
- Kernel básico vs. kernel shared.

Las dos variantes CUDA producen el mismo resultado físico dentro de la tolerancia establecida.

Para las validaciones de integración temporal y métricas de energía se establecieron los siguientes criterios:
- **Tolerancias:** Se utilizó `rtol = 1e-4` y `atol = 1e-8` para todas las comparaciones (Euler y Energías).
- **Justificación:** Al reutilizar la secuencia exacta de operaciones de `Integrator::eulerStep` en el host, la propagación del error de coma flotante se mantuvo idéntica entre la CPU y la GPU. Esto permitió mantener una tolerancia estricta del 0.01% incluso tras múltiples pasos iterativos, demostrando que ambas arquitecturas calculan el modelo físico con alta fidelidad y sin divergencia prematura.

---

### Cálculo de Energías en GPU

Se implementaron dos variantes CUDA para calcular la Energía Cinética ($K$) y la Energía Potencial ($U$) del sistema, llamadas a través de `NBodySimulator::calculateEnergyGpu(int method)`:

- `method = 0` (Reducción en memoria compartida): Cada hilo calcula su valor local y lo carga en `extern __shared__ double`. Luego, el bloque realiza una reducción paralela en forma de árbol binario. Finalmente, solo el hilo `0` de cada bloque utiliza `atomicAdd` para sumar el resultado parcial a la variable global.
- `method = 1` (Operaciones Atómicas): Cada hilo calcula su contribución individual (ya sea de $K$ o de $U$) y la suma directamente a una variable global en el device utilizando `atomicAdd`.

Ambas variantes fueron validadas contra la implementación serial de la CPU, demostrando generar resultados equivalentes dentro de la tolerancia permitida.

---

### Casos de prueba CUDA

Las pruebas implementadas cubren:

- `N = 2`.
- `N = 3`.
- `N = 31`.
- `N = 32`.
- `N = 33`.
- `N < blockDim.x`.
- `N` múltiplo de `blockDim.x`.
- `N` no múltiplo de `blockDim.x`.
- Último tile incompleto.
- Comparación CPU vs. kernel básico.
- Comparación CPU vs. kernel shared.
- Comparación kernel básico vs. kernel shared.
- Variante CUDA inválida.
- Tamaño de bloque inválido.
- Punteros device nulos.
- Ejecución con `N = 0`.

También se comprobaron los tamaños de bloque requeridos para los benchmarks:

```text
64
128
256
512
1024
```

Todos estos tamaños fueron válidos en la GPU utilizada durante el desarrollo local.

---

### GPU utilizada en desarrollo local

Las pruebas locales fueron ejecutadas mediante WSL2 sobre la siguiente GPU:

```text
NVIDIA GeForce RTX 3050 6GB Laptop GPU
Compute Capability: 8.6
```

El entorno local utilizado fue:

```text
Ubuntu 24.04 LTS mediante WSL2
CUDA Toolkit 12.8
nvcc 12.8
```

Las mediciones finales de rendimiento deben ejecutarse en el nodo GPU del clúster DIINF.

---

### Compilación

La arquitectura CUDA puede configurarse mediante la variable:

```makefile
CUDA_ARCH ?= 86
```

El valor predeterminado `86` corresponde a una GPU con capacidad de cómputo 8.6.

Para compilar utilizando otra arquitectura:

```bash
make cuda-test CUDA_ARCH=80
```

El valor debe modificarse de acuerdo con la GPU disponible en el clúster.

---

### Ejecución de tests CUDA

Para compilar y ejecutar solamente las pruebas CUDA:

```bash
cd nbody_2d
make cuda-test
```

Este comando:

1. Compila la prueba CUDA.
2. Compila los kernels básico y shared.
3. Ejecuta las comparaciones CPU/GPU.
4. Comprueba los casos de borde.
5. Verifica los tamaños de bloque.

---

### Ejecución de todos los tests

Para ejecutar primero los tests CPU y después los tests CUDA:

```bash
cd nbody_2d
make test-all
```

La suite CPU original mantiene:

```text
208 assertions en 6 casos de prueba
```

La suite CUDA comprueba ambas variantes y los casos de borde definidos anteriormente.

---

### Limpieza

Para eliminar los objetos y ejecutables generados:

```bash
make clean
```

Este objetivo elimina:

```text
Objetos C++
Objetos CUDA
Ejecutable principal
Ejecutable de tests CPU
Ejecutable de tests CUDA
```

---

### Sobrecargas preparadas para integración

En `NBodySystem` se declararon las sobrecargas requeridas por el Laboratorio 2:

```cpp
void computeAccelerationsGpu();
void computeAccelerationsGpu(int variant);
void computeAccelerationsGpu(int variant, int block_size);
```

La implementación definitiva de estas funciones se realizará al integrar la capa de buffers host/device.

La configuración esperada será:

```text
computeAccelerationsGpu()
    → configuración predeterminada

computeAccelerationsGpu(int variant)
    → selecciona kernel básico o shared

computeAccelerationsGpu(int variant, int block_size)
    → selecciona variante y tamaño de bloque
```

Estas funciones reutilizarán los lanzadores existentes, evitando duplicar la lógica física.


