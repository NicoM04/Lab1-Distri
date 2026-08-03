# Changelog

Todos los cambios notables de este proyecto se documentan en este archivo.

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/),
y este proyecto intenta adherirse a [Versionado Semántico](https://semver.org/lang/es/).

## [Unreleased]

Sin cambios adicionales registrados desde el cierre de la versión 2.0.0.

## [2.0.0] - 2026-08-03

Laboratorio 2: aceleración por GPU (CUDA) del simulador N-cuerpos 2D, con
flujo Git formalizado, integración continua extendida y agentes
automatizados de apoyo al repositorio.

### Added

- Kernel CUDA básico para el cálculo de aceleraciones gravitatorias, con
  un hilo por cuerpo (`computeAccelerationsKernel`).
- Kernel CUDA con memoria compartida (tiling) para reducir accesos a
  memoria global (`computeAccelerationsKernelShared`).
- Gestión RAII de memoria device mediante `CudaBuffer<T>`, usada por un
  layout SoA (`d_mass`, `d_x`, `d_y`, `d_vx`, `d_vy`, `d_ax`, `d_ay`).
- Integración de Euler ejecutada en GPU (`eulerStepGpu`,
  `NBodySystem::computeAccelerationsGpu`).
- Cálculo de energía cinética y potencial en GPU, mediante reducción en
  memoria compartida y mediante operaciones atómicas.
- Suite de pruebas CPU–GPU (`cuda_buffer_roundtrip_test`,
  `gpu_accelerations_smoke_test`, `test_gpu_integration`,
  `test_gpu_energy`), cubriendo casos de borde de tamaño de problema,
  tiles incompletos, punteros nulos y parámetros inválidos.
- Pipeline reproducible de benchmarking CUDA (`BenchmarkCuda`, flag
  `-benchmark-cuda`) con una matriz combinada de tamaño de problema (`N`),
  tamaño de bloque (`blockDim.x`), variante de kernel y modo de medición
  (kernel-only / extremo-a-extremo).
- Script de graficación (`lab2_plots/plot_real_data.py`) y job SLURM
  (`pipeline_lab2.slurm`) para ejecutar el pipeline completo en un nodo
  GPU de clúster.
- Imagen Docker base con CUDA Toolkit y Catch2, publicada en GHCR
  (`build_base_container.yml`), y workflow de integración continua
  (`ci.yml`) que compila la ruta GPU y ejecuta la suite CPU en cada
  push/PR.
- Flujo Git formalizado (rama `main` protegida, ramas
  `feature/*`/`fix/*`/`docs/*`, PR vinculada a issue, CI en verde,
  revisión humana antes de fusionar), documentado en
  `docs/git-and-releases.md`.
- Tres agentes automatizados (documentador, revisor de bugs, revisor de
  Pull Requests) con arquitectura de proveedor de IA configurable
  (`scripts/agents/common.py`), workflows de disparo propios
  (`.github/workflows/agent-*.yml`) y pruebas unitarias
  (`scripts/agents/tests/`).
- Documentación y evidencia de ejecución real de los agentes
  (`docs/agents-evidence.md`), y notas de release
  (`docs/release-notes-v2.0.0-lab2.md`).

### Changed

- Integración del simulador CPU/OpenMP existente con las variantes CUDA,
  manteniendo la ruta serial como referencia de corrección.
- `Makefile` y `Dockerfile` adaptados para compilar y probar la ruta CUDA
  junto con la ruta CPU existente.
- CI extendido para compilar los kernels y pruebas CUDA además de ejecutar
  la suite CPU en cada push/PR.
- README y documentación técnica actualizados para reflejar el diseño SoA,
  el contrato host/device, las tolerancias numéricas y el flujo Git del
  Laboratorio 2.
- Manejo de errores, deduplicación, límite semanal y clasificación
  mecánico/humano de los agentes endurecidos: límite de issues automáticos
  fail-closed, conteo por la etiqueta específica de cada agente, manejo
  unificado de errores HTTP, y garantía por código (no solo por prompt) de
  la frase de intervención humana y del recordatorio de que el merge es
  humano.
- Comentarios introductorios incorporados en los archivos señalados por el
  agente documentador (`CudaBuffer.h`, `CudaCheck.cuh`,
  `NBodySimulator.h`, `NBodySystem.h`).

### Fixed

- Llamadas a la API de CUDA sin la macro `CUDA_CHECK` en
  `NBodySimulator.cpp` y `kernels/metrics.cu`, detectadas por el agente
  revisor de bugs.
- Problemas de compilación CUDA e integración con CI para lograr una
  compilación estable en GitHub Actions sin requerir GPU en el runner.
- Ajustes al pipeline de benchmarking y graficación CUDA tras su primera
  integración.

[Unreleased]: https://github.com/NicoM04/Lab1-Distri/compare/v2.0.0-lab2...HEAD
[2.0.0]: https://github.com/NicoM04/Lab1-Distri/releases/tag/v2.0.0-lab2
