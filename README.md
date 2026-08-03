# Simulador N-Body 2D — OpenMP y CUDA

Simulador gravitatorio de N cuerpos en 2D, implementado en C++17. El
proyecto nació en el Laboratorio 1 como una versión serial y paralela con
OpenMP, y se extiende en el Laboratorio 2 con una ruta de cómputo en GPU
mediante CUDA. La versión CPU serial y la versión OpenMP se conservan
íntegras y se usan como referencia de corrección para la ruta GPU.

El código fuente vive en [`nbody_2d/`](nbody_2d/). El proyecto incluye
integración continua dockerizada, un flujo Git basado en Pull Requests con
revisión humana obligatoria, y un conjunto de agentes automatizados de
apoyo al mantenimiento del repositorio.

## Características principales

- Simulador N-cuerpos 2D con integración de Euler y cálculo de métricas
  físicas (energía cinética, potencial y total).
- **Laboratorio 1:** paralelización CPU con OpenMP, con análisis de
  scaling, comparación de schedules y Ley de Amdahl.
- **Laboratorio 2:** aceleración GPU con CUDA — kernel básico y kernel con
  memoria compartida, gestión RAII de memoria device (`CudaBuffer`),
  layout SoA, integración de Euler y cálculo de energía en GPU, validación
  numérica CPU–GPU, y un pipeline de benchmarking con ejecución opcional
  en clúster.
- Integración continua dockerizada, flujo Git con revisión humana
  obligatoria, y agentes automatizados de documentación, revisión de
  código y revisión de Pull Requests.

## Estructura del proyecto

```
nbody_2d/
├── main.cpp                 Punto de entrada y parseo de argumentos CLI
├── Particle.{h,cpp}          Partícula: masa, posición, velocidad, aceleración
├── NBodySystem.{h,cpp}       Cálculo de aceleraciones (serial, OpenMP, GPU)
├── NBodySimulator.{h,cpp}    Integración temporal (Euler) y energía
├── Integrator.{h,cpp}        Paso de integración de Euler
├── MetricsCalculator.{h,cpp} Métricas físicas del sistema
├── Benchmark.{h,cpp}         Benchmarks CPU/OpenMP (scaling, schedules)
├── BenchmarkCuda.{h,cpp}     Benchmarks CUDA (matriz N x blockDim)
├── Visualizer.{h,cpp}        Exportación de trayectorias y series de energía
├── CudaBuffer.h               Plantilla RAII para memoria device
├── CudaCheck.cuh               Macro CUDA_CHECK para comprobación de errores
├── kernels/                   Kernels CUDA (aceleraciones y energía)
├── tests/                     Pruebas Catch2 (CPU) y pruebas CUDA
├── lab2_plots/                Script de graficación de benchmarks CUDA
├── pipeline_lab2.slurm        Job SLURM para ejecución en clúster GPU
├── plot_reports.py            Gráficos de benchmarks OpenMP y análisis físico
├── Makefile
└── Dockerfile

docs/                         Documentación de flujo Git, releases y evidencia
scripts/agents/                Agentes automatizados (documentador, bugs, PR)
.github/workflows/             CI y workflows de los agentes
```

La lista anterior cubre los archivos fuente y carpetas relevantes; se omiten
objetos compilados (`*.o`) y binarios generados por `make`.

## Requisitos y entorno

- Compilador C++17 con soporte OpenMP (`g++`).
- CUDA Toolkit y `nvcc`: necesarios para **compilar** el proyecto completo,
  incluso si solo se usará la ruta CPU/OpenMP — los kernels CUDA se
  compilan y enlazan en el mismo ejecutable principal (probado con CUDA
  12.2 en la imagen Docker del proyecto y CUDA 12.8 en entornos de
  desarrollo local).
- GPU NVIDIA: necesaria únicamente para **ejecutar** (no para compilar) los
  kernels y las pruebas CUDA del Laboratorio 2.
- Catch2 v3 para las pruebas unitarias (ambos laboratorios).
- Python 3 con Matplotlib para los scripts de graficación
  (`plot_reports.py` para el Laboratorio 1, `lab2_plots/plot_real_data.py`
  para el Laboratorio 2).
- Docker, opcional, para compilar y ejecutar sin instalar dependencias
  localmente (ver más abajo).
- Un entorno con SLURM (como el clúster DIINF), solo si se ejecuta
  `pipeline_lab2.slurm`.

## Docker

```bash
docker pull ghcr.io/nicom04/lab1-distri:base

docker run --rm -v $(pwd):/workspace -w /workspace \
  ghcr.io/nicom04/lab1-distri:base bash -c "make clean && make all && make test"
```

La imagen (`nvidia/cuda:12.2.2-devel-ubuntu22.04` con Catch2 preinstalado)
se publica automáticamente vía `build_base_container.yml` cuando cambia el
`Dockerfile`, y es la misma que usa `ci.yml`. Compila el proyecto completo
(ambos laboratorios); ejecutar realmente los kernels CUDA requiere una GPU
física, no disponible en este contenedor genérico ni en GitHub Actions.

## Laboratorio 1 — CPU y OpenMP

### Compilación y pruebas

```bash
cd nbody_2d
make          # compila el ejecutable nbody_2d (requiere CUDA Toolkit para enlazar, no GPU)
make test     # compila y ejecuta la suite CPU (Catch2)
make clean    # elimina objetos y ejecutables generados
```

### Benchmarks y ejecución

```bash
make benchmark   # benchmark completo, scaling (1/2/4/8 hilos) y comparación de schedules
make analysis    # simulación física, exporta trayectorias y energía
```

Ambos targets aceptan parámetros manuales del binario (`-N`, `-threads`,
`-schedule`, `-chunk`, `-steps`, `-sample`, entre otros); ver `main.cpp`
para el listado completo.

### Resultados generados

- `Resultados_Benchmark/*.dat` y `performance_plots.png`: tiempo, speedup,
  eficiencia y comparación contra la Ley de Amdahl (`make benchmark`).
- `Resultados_Analisis/*.dat` (`trajectories.dat`, `energy_timeseries.dat`)
  y `physics_plots.png`: simulación física (`make analysis`).

## Laboratorio 2 — GPU y CUDA

### Compilación y pruebas

```bash
cd nbody_2d
make cuda-build   # compila las pruebas CUDA (no las ejecuta)
make cuda-test    # compila y ejecuta la suite CUDA (requiere GPU NVIDIA)
make test-all     # make test seguido de make cuda-test
```

La arquitectura CUDA de compilación se controla con `CUDA_ARCH` (valor por
defecto `75`, compute capability 7.5):

```bash
make cuda-test CUDA_ARCH=80
```

El workflow de integración continua (`ci.yml`) ejecuta
`make clean && make all && make cuda-build` y luego `make test` dentro de
la imagen base publicada en GHCR. Esto compila los kernels y las pruebas
CUDA en cada push/PR, pero **no las ejecuta**: los runners de GitHub
Actions no tienen GPU disponible, por lo que solo la suite CPU corre
realmente en CI.

### Benchmarks y ejecución

```bash
./nbody_2d -benchmark-cuda [-iters N]
```

Ejecuta la matriz de benchmarking exigida por el laboratorio:

- `N`: 256, 512, 1024, 2000.
- `blockDim.x`: 64, 128, 256, 512, 1024.
- Variante de kernel: básica y con memoria compartida.
- Modo de medición: kernel-only y extremo-a-extremo.
- Repeticiones por punto: 10 por defecto (parámetro `-iters`).

Los resultados se exportan a `lab2_plots/benchmark_results.dat`. El
gráfico consolidado se genera con:

```bash
cd lab2_plots
python3 plot_real_data.py   # produce performance_plots.png
```

### Clúster DIINF

`pipeline_lab2.slurm` automatiza compilación, benchmark CUDA, simulación
física y graficado en un nodo GPU vía SLURM:

```bash
sbatch pipeline_lab2.slurm
squeue -u <usuario>
```

El script está preparado para el nodo GPU del clúster DIINF
(`--partition=GPU` en la cabecera SLURM) y documentado en
[`nbody_2d/README.md`](nbody_2d/README.md).

### Validación numérica

La versión CPU serial se conserva como referencia de corrección para todas
las rutas GPU. Las comparaciones numéricas usan:

```
rtol = 1e-4
atol = 1e-8
```

aplicado como `|resultado_gpu - resultado_cpu| <= atol + rtol × |resultado_cpu|`,
para:

- Aceleraciones: CPU serial vs. kernel básico, CPU serial vs. kernel
  shared, y kernel básico vs. kernel shared.
- Integración de Euler en GPU frente a la versión CPU.
- Energía cinética y potencial en GPU (ambos métodos de reducción) frente
  a la versión CPU.

La suite CPU (Catch2) mantiene 208 assertions en 6 casos de prueba. La
suite CUDA cubre además casos de borde: `N = 0`, `N` menor, igual y no
múltiplo de `blockDim.x`, tiles incompletos, punteros device nulos,
tamaño de bloque y variante inválidos.

### Resultados generados

- `lab2_plots/benchmark_results.dat` y `lab2_plots/performance_plots.png`:
  matriz de benchmarking CUDA (`-benchmark-cuda` + `plot_real_data.py`).
- `lab2_plots/trajectories.dat` y `lab2_plots/energy_timeseries.dat`:
  generados por `pipeline_lab2.slurm` al ejecutar el pipeline completo en
  el clúster.

Todos los archivos anteriores son artefactos generados por `make`/los
scripts y no están versionados en el repositorio (ver `.gitignore`).

## Flujo Git

`main` está protegida: todo cambio llega mediante una Pull Request vinculada
a un issue, con el pipeline de CI en verde y revisión humana antes de
fusionar. Las ramas de trabajo usan los prefijos `feature/*`, `fix/*` y
`docs/*`, y se eliminan una vez fusionadas. El detalle completo del flujo,
incluyendo el rol de los agentes automatizados dentro de este proceso, está
en [`docs/git-and-releases.md`](docs/git-and-releases.md).

## Organización del equipo

| Rol | Integrante | Responsabilidades |
|---|---|---|
| 1. Kernels CUDA | Francisco Riquelme | Kernels de aceleraciones (básico y shared), manejo de índices, validación de bordes y macros `CUDA_CHECK`. |
| 2. Host/device y memoria | Gabriel Cabrera | Diseño de `CudaBuffer`, layout SoA, minimización de transferencias `cudaMemcpy` por paso temporal. |
| 3. Integración y validación | Amaru Monje | Integrador de Euler sincronizado con device, validación CPU vs. GPU, kernels de reducción de energía. |
| 4. Git, releases y agentes | Nicolás Morales | Flujo Git y protección de `main`, `CHANGELOG.md`, configuración de los agentes automatizados. |
| 5. Calidad, CI y visualización | Thomas Gustafsson | Makefile y Dockerfile, configuración de CI, pipeline de benchmarking y generación de gráficos. |

## Agentes automatizados de apoyo

El repositorio incluye tres agentes de apoyo, implementados en
[`scripts/agents/`](scripts/agents/) y disparados por los workflows en
[`.github/workflows/`](.github/workflows/): un documentador, un revisor de
bugs y un revisor de Pull Requests. Ninguno de los tres aprueba, fusiona ni
hace push a `main`; el merge siempre lo realiza una persona.

| Agente | Disparador | Entrada | Salida | Mecánico | Requiere intervención humana |
|---|---|---|---|---|---|
| Documentador | Semanal, manual, o push a `main` que toque documentación | README(s), `CHANGELOG.md` | Issue etiquetado | Enlace roto, encabezado o plantilla evidente | Explicar kernels, memoria, física o sincronización |
| Revisor de bugs | Diario o manual | Código de `nbody_2d/`, resultado de CI | Issue etiquetado (con parche sugerido si aplica) | CUDA sin `CUDA_CHECK`, archivo generado versionado | Física, API pública, kernels, sincronización no trivial, CI en fallo |
| Revisor de PR | Al terminar el CI de una PR, o manual | Resultado de CI, diff de la PR | Comentario en la PR | Solo documentación/formato/configuración, con issue vinculado | Cambios de física, kernels, memoria CUDA, o CI en fallo |

Los tres scripts admiten una capa de proveedor de IA configurable
(`scripts/agents/common.py`): un endpoint externo compatible con OpenAI, o
GitHub Models usando el `GITHUB_TOKEN` de Actions. Cuando ningún proveedor
está disponible, el script no simula una respuesta: usa un análisis
estático determinístico basado en sus propias heurísticas, identificado
literalmente como `ANÁLISIS ESTÁTICO (sin modelo de IA disponible)`. Ese
análisis estático no equivale a la salida de un modelo generativo.

Los agentes ya se ejecutaron de forma real en GitHub Actions y, en esas
ejecuciones, no había proveedor de IA configurado, por lo que usaron el
fallback estático. El resultado fue real de todas formas: el documentador
abrió los issues #10 a #13 por encabezados sin comentario introductorio, el
revisor de bugs abrió los issues #14 a #17 por llamadas CUDA sin
`CUDA_CHECK`, y el revisor de PR comentó las PR #18, #19, #21 y #23
indicando si el cambio requería revisión humana. El detalle completo de
estas ejecuciones, con las URLs verificadas de cada issue, PR y ejecución
de Actions, está en [`docs/agents-evidence.md`](docs/agents-evidence.md).

## Release

La entrega final del Laboratorio 2 se publica mediante el tag
`v2.0.0-lab2`, creado desde `main` una vez fusionados los cambios
pendientes. El historial de cambios está en [`CHANGELOG.md`](CHANGELOG.md)
y las notas de la release en
[`docs/release-notes-v2.0.0-lab2.md`](docs/release-notes-v2.0.0-lab2.md).
El procedimiento detallado de release está en
[`docs/git-and-releases.md`](docs/git-and-releases.md).
