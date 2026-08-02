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

## Roles del equipo — Laboratorio 2

Roles inferidos a partir de las ramas y commits fusionados en `main`
(`feature/cuda-acceleration-kernels`, `feature/cuda-buffer`,
`feature/integration-gpu`, `feature/ci-cuda-env`). Confirmar/corregir con el
equipo si algún dato no coincide.

| Rol | Encargado | Área |
|---|---|---|
| Rol 1 | Francisco Riquelme | Kernels CUDA de aceleraciones (básico y shared) |
| Rol 2 | Gabriel Cabrera | `CudaBuffer` RAII y gestión de memoria host/device |
| Rol 3 | Amaru Monje | Integración Euler GPU y cálculo de energía |
| Rol 4 | Nicolás Morales | Git, releases y agentes de IA (este documento) |
| Rol 5 | Thomas Gustafsson *(inferido, confirmar)* | CI, Docker y calidad |

## Flujo Git (Laboratorio 2)

Resumen — ver `docs/git-and-releases.md` para el detalle completo:

- `main` está **protegida**: no se permite push ni commits directos.
- Todo cambio se hace en una rama `feature/*` o `fix/*`.
- La PR hacia `main` debe vincular un issue (`Closes #<numero>`).
- La PR debe tener el CI (`ci.yml`) en verde y pasar revisión humana antes
  de fusionarse.
- Tras fusionar, la rama se elimina.
- Ningún agente de IA aprueba ni fusiona PRs; el merge siempre lo hace una
  persona.

## Agentes de IA (Rol 4)

Tres agentes de solo lectura sobre el código (con permiso de escritura
limitado a issues/comentarios), implementados en `scripts/agents/` y
disparados por los workflows en `.github/workflows/agent-*.yml`. Ninguno
hace merge ni escribe en `main`. Detalle de diseño en
`scripts/agents/README.md`; procedimiento y flujo completo (issue → rama →
PR → CI → comentario del agente → revisión y merge humano) en
[`docs/git-and-releases.md`](docs/git-and-releases.md).

**Estado real:** los tres agentes están versionados en `main` y **ya se
ejecutaron mediante GitHub Actions** (no solo en `--dry-run` local). Esas
ejecuciones observadas usaron el **fallback de análisis estático** de
`scripts/agents/common.py` (el proveedor de IA no estaba configurado en ese
momento), por lo que cada issue/comentario generado comenzó literalmente
con `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**`. Ese análisis
estático **no es la salida de un modelo de IA generativo**: es la
heurística determinística del propio script (enlaces rotos, comentarios de
cabecera ausentes, llamadas CUDA sin `CUDA_CHECK`, archivos tocados en el
diff de una PR). Aun así, el resultado fue real: el documentador y el
revisor de bugs abrieron issues reales que el equipo corrigió, y el
revisor de PR publicó comentarios reales indicando si una PR requería
revisión humana. La evidencia verificable de estas ejecuciones (issues,
PRs y su correspondencia con hallazgos concretos) está en
[`docs/agents-evidence.md`](docs/agents-evidence.md). En ningún caso un
agente aprobó, fusionó o hizo push a `main`: la aprobación y el merge de
cada PR siguieron siendo una acción humana.

- El **documentador** produjo issues por encabezados de archivos
  CUDA/host sin comentario introductorio (ver issues #10-#13).
- El **revisor de bugs** produjo issues por llamadas CUDA sin `CUDA_CHECK`
  (ver issues #14-#17).
- El **revisor de PR** comentó PRs indicando cuándo un cambio requería
  revisión humana por tocar física/kernels/memoria CUDA, o por estar fuera
  del alcance mecánico permitido (ver PR #18 y #23), recordando siempre que
  el merge lo realiza una persona.

| Agente | Herramienta | Disparador / frecuencia | Entradas | Salida | Criterio mecánico | Criterio humano | Permisos |
|---|---|---|---|---|---|---|---|
| Documentador | `scripts/agents/documenter.py` | `schedule` semanal (lunes), `workflow_dispatch`, push a `main` que toque README/CHANGELOG | README.md, nbody_2d/README.md, nbody_2d/tests/README.md, CHANGELOG.md | Issue etiquetado `documentation` + `agent-documenter` | Typo, enlace roto, encabezado faltante, plantilla evidente | Explicar kernels, memoria, física, sincronización o tolerancias → `Requiere intervención humana: <motivo>` (garantizado por código, no solo por prompt) | `contents: read`, `issues: write`, `models: read` |
| Revisor de bugs | `scripts/agents/bug_reviewer.py` | `schedule` diario, `workflow_dispatch` | Código fuente de `nbody_2d/`, `git ls-files`, última conclusión de `ci.yml` en `main` (vía API, sin re-ejecutar tests) | Issue etiquetado `bug` + `agent-bug-reviewer` (con parche sugerido si es mecánico) | CUDA sin `CUDA_CHECK`, archivo generado versionado | Física, API pública, orden del integrador, lógica de kernels, reducción/sincronización no trivial, o CI en fallo → `Requiere intervención humana: <motivo>` (garantizado por código) | `contents: read`, `issues: write`, `models: read`, `actions: read` |
| Revisor de PR | `scripts/agents/pr_reviewer.py` | `workflow_run` al terminar `CI` (`ci.yml`), `workflow_dispatch` manual | Resultado de CI, diff de la PR, descripción de la PR | Comentario en la PR (uno por commit) | CI verde, solo doc/formato/config evidente, sin tocar física/kernels/API pública, con issue vinculado (`Closes #N`/`Fixes #N`/`Resolves #N`/`Refs #N`, etc.) | Cualquier otro caso, o CI en fallo → revisión humana (garantizado por código: nunca se omite el recordatorio de que el merge es humano) | `contents: read`, `issues: write`, `pull-requests: write`, `models: read` |

### Etiquetas requeridas antes de la primera ejecución real

GitHub rechaza la creación de un issue si alguna etiqueta indicada no existe
en el repositorio. Antes de la primera ejecución real (no `--dry-run`) del
documentador o del revisor de bugs, una persona debe verificar/crear en
GitHub:

- `documentation` (label por defecto de GitHub; **confirmar que exista**)
- `agent-documenter` (custom; casi seguro **no existe todavía**)
- `bug` (label por defecto de GitHub; **confirmar que exista**)
- `agent-bug-reviewer` (custom; casi seguro **no existe todavía**)

El conteo del límite semanal (máximo 5 issues automáticos por agente cada 7
días, ver más abajo) usa la etiqueta específica de cada agente
(`agent-documenter`/`agent-bug-reviewer`), no la etiqueta genérica
compartida, para no contar issues creados manualmente por personas.

Si falta una etiqueta (o cualquier otro error de la API ocurre al crear un
issue), el agente registra un mensaje claro en el log y **continúa
procesando los hallazgos restantes** en la misma ejecución; no se detiene
con un error sin manejar.

### Proveedor de IA y secretos

No hay ningún proveedor de IA configurado en este repositorio; por eso las
ejecuciones reales observadas hasta ahora (ver
[`docs/agents-evidence.md`](docs/agents-evidence.md)) usaron el fallback de
análisis estático descrito arriba, no un modelo generativo. La capa
`scripts/agents/common.py` es agnóstica de proveedor:

1. Si existen los secrets `AGENT_API_URL` y `AGENT_API_KEY` (y opcionalmente
   `AGENT_MODEL`), se usa ese endpoint compatible con OpenAI.
2. Si no, se intenta GitHub Models usando el `GITHUB_TOKEN` automático de
   Actions (requiere que GitHub Models esté habilitado para el repositorio
   y el permiso `models: read`; **esto debe confirmarlo una persona con
   acceso a la organización/repositorio**, no fue verificado como parte de
   este trabajo).
3. Si ninguno está disponible, el agente falla con un mensaje claro (o solo
   corre en `--dry-run` con un análisis estático sin IA) — nunca simula una
   respuesta.

No se requiere ningún secreto nuevo para que los workflows *existan y hagan
`--dry-run`*; si se quiere un proveedor externo, agregar `AGENT_API_URL` y
`AGENT_API_KEY` como *secrets* del repositorio (Settings → Secrets and
variables → Actions).

### Ejecutar los agentes manualmente

Desde la pestaña **Actions** de GitHub, cada workflow `agent-*` tiene
`workflow_dispatch` con un input `dry_run`. También se pueden correr en
local (ver `scripts/agents/README.md`):

```bash
export GITHUB_TOKEN=...   # o usar --dry-run
export GITHUB_REPOSITORY=NicoM04/Lab1-Distri
python scripts/agents/documenter.py --dry-run
python scripts/agents/bug_reviewer.py --dry-run
python scripts/agents/pr_reviewer.py --pr-number <n> --sha <sha> --conclusion success --dry-run
```

`--dry-run` (o `AGENT_DRY_RUN=1`) evita cualquier escritura en GitHub
(issues o comentarios); solo imprime lo que habría hecho.

### Límites y garantías

- Máximo **5 issues automáticos por agente (etiqueta `agent-documenter`/
  `agent-bug-reviewer`) cada 7 días** sin revisión humana; superado el
  límite, el agente lo indica en el log y no crea más.
- El límite es **fail-closed**: si no se puede verificar el conteo semanal
  (p. ej. error de red o de la API de GitHub), el agente **no crea el
  issue** y registra "Requiere intervención humana: revisar la API de
  GitHub" — nunca asume que el conteo es cero.
- **Ningún agente fusiona ni aprueba PRs.** El merge es siempre una acción
  humana; el revisor de PR fuerza este recordatorio por código en cada
  comentario, sin depender de que el modelo de IA lo incluya.
- Los issues y comentarios usan marcadores HTML identificables para no
  duplicar el mismo hallazgo/commit.

### Procedimiento de release

Ver `docs/git-and-releases.md` (sección 5) para el detalle. Resumen:

1. Mover las entradas de `CHANGELOG.md` de `[Unreleased]` a `[2.0.0] -
   YYYY-MM-DD`.
2. Confirmar que `ci.yml` está en verde sobre `main`.
3. Fusionar las PR pendientes que deban incluirse.
4. Crear el tag `v2.0.0-lab2` desde `main`.
5. Publicar las notas de release usando `docs/release-notes-v2.0.0-lab2.md`
   como base.

**Esto todavía no se ha hecho.** No existe el tag `v2.0.0-lab2` ni la
release; `docs/release-notes-v2.0.0-lab2.md` es un borrador.


