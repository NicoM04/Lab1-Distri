# Notas de release — v2.0.0-lab2 (BORRADOR)

> **BORRADOR — NO PUBLICADO.** Este archivo es un borrador de notas de
> release. El tag `v2.0.0-lab2` todavía no ha sido creado y esta release
> todavía no ha sido publicada en GitHub. No usar este contenido como
> anuncio oficial hasta que se retire esta marca.

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

## Agentes de IA (Rol 4)

- Agente documentador: revisa README(s) y `CHANGELOG.md`, abre issues
  etiquetados ante documentación faltante/desactualizada o enlaces rotos.
- Agente revisor de bugs: análisis diario de `main` en busca de patrones de
  riesgo (llamadas CUDA sin comprobación, tests rotos, archivos generados
  versionados, tolerancias inconsistentes), escalando a humanos los cambios
  que toquen física, API pública o lógica de kernels.
- Agente revisor de PR: comenta automáticamente cada PR tras finalizar su
  CI, clasificando el cambio como mecánico o que requiere revisión humana,
  sin aprobar ni fusionar nunca.
- Los tres agentes están implementados con una capa de proveedor de IA
  configurable (`scripts/agents/common.py`), soportan `--dry-run` y
  respetan un límite de 5 issues automáticos por semana sin revisión
  humana.

## Pruebas

- Suite CPU (Catch2) heredada del Laboratorio 1: 208 assertions en 6 casos.
- Suite CUDA: `cuda_buffer_roundtrip_test`, `gpu_accelerations_smoke_test`,
  `test_gpu_integration`, `test_gpu_energy`, cubriendo casos de borde de
  tamaño de problema, tiles incompletos y comparación CPU/GPU.

## Pendiente / fuera de alcance de esta release (Rol 5 y otros)

- Mediciones de rendimiento en el nodo GPU del clúster DIINF (las mediciones
  locales fueron hechas en una RTX 3050 vía WSL2 y quedan documentadas como
  referencia, no como resultado final de cluster).
- Cualquier entregable específico del Rol 5 (calidad/CI/visualización más
  allá de lo ya fusionado) que no esté reflejado en el historial de `main`
  al momento de este borrador.
- Evidencia real de ejecución de los agentes de IA: ver
  `docs/agents-evidence.md`, todavía sin casos registrados.
- Publicación efectiva del tag y de la release `v2.0.0-lab2` (ver
  `docs/git-and-releases.md`, sección 5, para el procedimiento).
