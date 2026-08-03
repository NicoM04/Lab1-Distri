# Notas de release — v2.0.0-lab2

## Resumen

Laboratorio 2: aceleración por GPU (CUDA) del simulador N-cuerpos 2D
desarrollado en el Laboratorio 1, más la formalización del flujo Git,
releases y agentes de IA de apoyo al equipo.

## Cambios CUDA

- Kernels de aceleraciones gravitatorias en dos variantes: básica y con
  memoria compartida (tiling), cada una con su propio lanzador y ambas
  compartiendo el mismo contrato host/device (sin `cudaMalloc`/`cudaFree`
  internos, sin transferencias, sin sincronización interna).
- Índice global de cuerpo por hilo (`blockIdx.x * blockDim.x + threadIdx.x`)
  y cálculo de grilla por división a techo.
- Comprobación de errores mediante la macro `CUDA_CHECK` y
  `cudaGetLastError()` tras cada lanzamiento.

## Cambios de memoria

- Layout SoA (`d_mass`, `d_x`, `d_y`, `d_vx`, `d_vy`, `d_ax`, `d_ay`) para
  favorecer accesos coalescentes.
- Clase RAII `CudaBuffer<T>` para reservar/liberar memoria device de forma
  automática y segura.
- Conteo explícito de transferencias por paso (`deviceTransferCount()`) para
  hacer trazable el costo de las copias H2D/D2H.

## Integración y energía

- `eulerStepGpu` para ejecutar el paso de integración de Euler en GPU.
- `NBodySystem::computeAccelerationsGpu(...)` como ruta completa (kernel +
  sincronización + descarga de resultados).
- `NBodySimulator::calculateEnergyGpu(int method)` con dos estrategias:
  reducción en memoria compartida + `atomicAdd` (`method = 0`) y acumulación
  atómica directa (`method = 1`).
- Validación CPU vs. GPU con `rtol = 1e-4` / `atol = 1e-8` para
  aceleraciones, integración y energía.

## CI y Docker

- Imagen base Docker (`nvidia/cuda:12.2.2-devel-ubuntu22.04` + Catch2 v3)
  publicada en GHCR mediante `build_base_container.yml`.
- `ci.yml` compila (`make all && make cuda-build`) y ejecuta `make test`
  dentro de esa imagen en cada push/PR, sin requerir GPU física en el
  runner.

## Pipeline de benchmarks y gráficos (Rol 5)

- `nbody_2d/BenchmarkCuda.cpp`/`.h`, expuesto vía el flag `-benchmark-cuda`
  de `main.cpp`, con una **matriz combinada** de medición: para cada
  tamaño de problema `N` (`256, 512, 1024, 2000`) y cada `blockDim.x`
  (`64, 128, 256, 512, 1024`), mide tiempo CPU serial y tiempo GPU (kernel
  básico y shared, tanto solo-kernel como extremo-a-extremo), exportando
  todo a `benchmark_results.dat`.
- `nbody_2d/lab2_plots/plot_real_data.py` genera los gráficos finales a
  partir de esos datos.
- `nbody_2d/pipeline_lab2.slurm`: script SLURM (`--partition=GPU`) para
  ejecutar compilación, benchmark, simulación física y graficado completo
  en el nodo GPU del clúster DIINF.
- El pipeline se ejecutó en el nodo GPU del clúster DIINF mediante
  `pipeline_lab2.slurm`, y sus resultados se utilizaron para el análisis
  final de rendimiento del laboratorio. El pipeline y el código que los
  produce están fusionados en `main`; los artefactos generados (`.dat`,
  `.png`) no se versionan en el repositorio, ya que están excluidos por
  `.gitignore`.

## Agentes de IA (Rol 4)

- Agente documentador: revisa README(s) y `CHANGELOG.md`, abre issues
  etiquetados ante enlaces rotos o encabezados de archivo sin comentario
  introductorio.
- Agente revisor de bugs: análisis diario de `main` en busca de patrones de
  riesgo (llamadas CUDA sin comprobación, archivos generados versionados,
  tolerancias inconsistentes, última conclusión de `ci.yml`), escalando a
  humanos los cambios que toquen física, API pública o lógica de kernels.
- Agente revisor de PR: comenta automáticamente cada PR tras finalizar su
  CI, clasificando el cambio como mecánico o que requiere revisión humana,
  sin aprobar ni fusionar nunca.
- Los tres agentes están implementados con una capa de proveedor de IA
  configurable (`scripts/agents/common.py`), soportan `--dry-run`, respetan
  un límite de 5 issues automáticos por semana (fail-closed) por agente, y
  garantizan por código —no solo por prompt— la frase "Requiere
  intervención humana" y el recordatorio de que el merge es humano.
- **Ejecución real:** los tres agentes ya se ejecutaron en GitHub Actions.
  El documentador y el revisor de bugs abrieron issues reales (#10-#13 y
  #14-#17 respectivamente) que el equipo corrigió (commits `3ecf7b7` y
  `fbabdee`); el revisor de PR comentó las PR #18, #19, #21 y #23. En
  todos los casos observados se usó el fallback de análisis estático (sin
  proveedor de IA configurado), no un modelo generativo. Las seis
  evidencias mínimas (dos por agente), con sus URLs de ejecución de GitHub
  Actions verificadas manualmente por el equipo, ya están registradas en
  `docs/agents-evidence.md`.

## Pruebas

- Suite CPU (Catch2) heredada del Laboratorio 1: 208 assertions en 6 casos.
- Suite CUDA: `cuda_buffer_roundtrip_test`, `gpu_accelerations_smoke_test`,
  `test_gpu_integration`, `test_gpu_energy`, cubriendo casos de borde de
  tamaño de problema, tiles incompletos y comparación CPU/GPU.

## Limitaciones conocidas

- No se configuró un proveedor de IA generativa para esta entrega. Las
  ejecuciones reales de los tres agentes automatizados, registradas en
  `docs/agents-evidence.md`, usaron el fallback de análisis estático
  determinístico (`ANÁLISIS ESTÁTICO (sin modelo de IA disponible)`) — no
  un modelo de IA generativo. Los issues y comentarios de esa evidencia sí
  fueron creados realmente por GitHub Actions; esto se documenta de forma
  transparente en `docs/agents-evidence.md`. Los agentes soportan una
  interfaz configurable (`AGENT_API_URL`/`AGENT_API_KEY`, o GitHub Models
  vía `GITHUB_TOKEN`) para conectar un proveedor generativo en el futuro.
- Las mediciones locales documentadas en `nbody_2d/README.md` (RTX 3050 vía
  WSL2) corresponden al entorno de desarrollo y quedan como referencia,
  distintas de los resultados oficiales obtenidos en el clúster DIINF.
