# Changelog

Todos los cambios notables de este proyecto se documentan en este archivo.

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/),
y este proyecto intenta adherirse a [Versionado Semántico](https://semver.org/lang/es/).

## [Unreleased]

Cuando se publique la release, estas entradas se moverán a una sección
`## [2.0.0] - YYYY-MM-DD` (ver `docs/release-notes-v2.0.0-lab2.md` para el
borrador de notas y `docs/git-and-releases.md` para el procedimiento). El tag
`v2.0.0-lab2` todavía **no** ha sido creado.

### Added

- Kernels CUDA de aceleraciones gravitatorias: variante básica
  (`computeAccelerationsKernel`) y variante con memoria compartida
  (`computeAccelerationsKernelShared`), con lanzadores configurables por
  tamaño de bloque (`kernels/accelerations.cu`, `kernels/accelerations.cuh`).
- Clase RAII `CudaBuffer<T>` para gestión de memoria device (`cudaMalloc`/
  `cudaFree` automáticos) usada por el layout SoA (`d_mass`, `d_x`, `d_y`,
  `d_vx`, `d_vy`, `d_ax`, `d_ay`) (`nbody_2d/CudaBuffer.h`).
- Macro `CUDA_CHECK` para comprobación de errores de la API CUDA
  (`nbody_2d/CudaCheck.cuh`).
- Integración Euler en GPU (`eulerStepGpu`) y ruta completa
  `NBodySystem::computeAccelerationsGpu(...)` (kernel + sincronización +
  transferencias D2H), con contador `deviceTransferCount()` para trazabilidad
  de copias host/device.
- Cálculo de energía cinética y potencial en GPU
  (`NBodySimulator::calculateEnergyGpu(int method)`) con dos estrategias:
  reducción en memoria compartida + `atomicAdd` (`method = 0`) y acumulación
  puramente atómica (`method = 1`).
- Pruebas CUDA: `cuda_buffer_roundtrip_test`, `gpu_accelerations_smoke_test`,
  `test_gpu_integration`, `test_gpu_energy`, cubriendo casos de borde (`N=0`,
  `N` no múltiplo de `blockDim.x`, tiles incompletos, punteros nulos, tamaños
  de bloque inválidos) y comparaciones CPU vs. GPU con `rtol = 1e-4`,
  `atol = 1e-8`.
- Target `cuda-test` y `test-all` en el `Makefile` para compilar/ejecutar la
  suite CUDA junto a la suite CPU existente.
- `Dockerfile` basado en `nvidia/cuda:12.2.2-devel-ubuntu22.04` con Catch2 v3
  preinstalado, usado como imagen base publicada en GHCR.
- Workflow `build_base_container.yml`: construye y publica la imagen base en
  GHCR cuando cambia `Dockerfile`, y hace una compilación de humo dentro del
  contenedor.
- Workflow `ci.yml`: en cada push/PR, compila (`make all && make cuda-build`)
  y ejecuta `make test` dentro de la imagen base publicada en GHCR.
- `CHANGELOG.md` siguiendo Keep a Changelog (este archivo).
- Documentación del flujo Git y de releases en `docs/git-and-releases.md`.
- Tres agentes de IA en `scripts/agents/` (documentador, revisor de bugs y
  revisor de pull requests) con arquitectura de proveedor de IA
  configurable (`scripts/agents/common.py`), soporte `--dry-run` y límite de
  5 issues automáticos por semana.
- Workflows de disparo para los agentes de IA:
  `.github/workflows/agent-documenter.yml`,
  `.github/workflows/agent-bug-reviewer.yml`,
  `.github/workflows/agent-pr-reviewer.yml`.
- Plantilla `docs/agents-evidence.md` para registrar evidencia real de
  ejecución de los agentes.
- Borrador `docs/release-notes-v2.0.0-lab2.md` para la futura release.

### Changed

- `README.md` actualizado en las ramas de kernels CUDA, `CudaBuffer` e
  integración/energía GPU para documentar el diseño SoA, el contrato
  host/device, las tolerancias numéricas y los comandos de compilación CUDA.
- `nbody_2d/README.md` actualizado para documentar el layout SoA en memoria
  device y el ciclo de transferencias `CudaBuffer<T>`.
- `.gitignore` (raíz y `nbody_2d/`) actualizado para excluir binarios y
  ejecutables de las pruebas CUDA (`cuda_buffer_roundtrip_test`,
  `gpu_accelerations_smoke_test`, `gpu_integration_test`, `gpu_energy_test`).
- Flujo de trabajo Git formalizado para el Laboratorio 2: rama `main`
  protegida, ramas `feature/*`/`fix/*`, PR vinculada a issue, CI en verde y
  revisión humana antes de fusionar (ver `docs/git-and-releases.md`).

### Fixed

- Múltiples correcciones al pipeline de CI para lograr una compilación CUDA
  estable en GitHub Actions sin requerir ejecución real de GPU en el runner
  (ver commits `Arreglo compilación CUDA en CI sin requerir ejecución GPU` y
  los sucesivos `Fixes CI` en el historial de `main`).

[Unreleased]: https://github.com/NicoM04/Lab1-Distri/compare/main...HEAD
