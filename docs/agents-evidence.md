# Evidencia real de ejecución de los agentes de IA

Este archivo registra evidencia **real** (no simulada) de ejecuciones de los
tres agentes de IA del Rol 4. Cada fila se completa solo con datos que pueden
sustentarse en el repositorio (commits, PRs fusionadas, historial de `main`)
o que fueron reportados directamente por el equipo. Cuando una URL no estaba disponible, se dejaba temporalmente marcada como pendiente.

**Proveedor de IA en las ejecuciones registradas:** en todos los casos de
esta tabla, la ejecución real utilizó el **fallback estático** de
`scripts/agents/common.py` (no había proveedor de IA configurado), es decir
que el cuerpo de cada issue/comentario comenzó literalmente con:

```
**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**
```

Esto no equivale a la salida de un modelo generativo: es el análisis
determinístico basado en las heurísticas de cada script (enlaces rotos,
comentarios de cabecera ausentes, llamadas CUDA sin `CUDA_CHECK`, archivos
tocados en el diff), tal como está diseñado para degradar de forma segura
cuando no hay `AGENT_API_URL`/`AGENT_API_KEY` ni GitHub Models disponibles.

Los issues y comentarios listados abajo fueron **creados realmente por la
identidad `github-actions` dentro de ejecuciones reales de GitHub Actions**
(no simulados ni redactados manualmente), autenticados con el `GITHUB_TOKEN`
de cada workflow. Las URLs de ejecución (`/actions/runs/<id>`) fueron
verificadas manualmente por el equipo con acceso al repositorio en GitHub.

Se requieren como mínimo **dos casos reales por agente**; esta tabla ya
cumple ese mínimo para los tres agentes.

## Agente documentador

El documentador detectó, con su heurística `find_undocumented_headers`
(`scripts/agents/documenter.py`), encabezados de archivos CUDA/host sin
comentario introductorio en las primeras líneas. Ese hallazgo se corrigió
en el commit `3ecf7b7` ("docs(cuda): comentarios faltantes indicados por
agent-documenter"), que agrega un bloque `/** @file ... */` a
`nbody_2d/CudaBuffer.h`, `nbody_2d/CudaCheck.cuh`, `nbody_2d/NBodySimulator.h`
y `nbody_2d/NBodySystem.h` — exactamente los 4 archivos que la heurística
del agente señala. Ese commit se fusionó a `main` mediante el **PR #19**
(rama `docs/code-comments`, merge commit `01ea9a1`).

| Fecha | Issue | URL del issue | URL de ejecución (Actions) | Hallazgo / análisis producido | Clasificación | Acción humana posterior | Resultado final |
|---|---|---|---|---|---|---|---|
| 2026-07-30 o antes (resuelto por el commit `3ecf7b7`, 2026-07-30 16:49 -04:00) | #10 | https://github.com/NicoM04/Lab1-Distri/issues/10 | https://github.com/NicoM04/Lab1-Distri/actions/runs/30509741278 | `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**` — encabezado sin comentario introductorio en `nbody_2d/CudaBuffer.h` | Humano (`Requiere intervención humana: ...`, comentario de cabecera requiere explicar diseño) | Amaru Monje agregó el comentario `/** @file ... */` correspondiente | Corregido y fusionado vía PR #19 (commit `3ecf7b7`) |
| 2026-07-30 o antes (resuelto por el commit `3ecf7b7`, 2026-07-30 16:49 -04:00) | #13 | https://github.com/NicoM04/Lab1-Distri/issues/13 | https://github.com/NicoM04/Lab1-Distri/actions/runs/30509741278 | `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**` — encabezado sin comentario introductorio en `nbody_2d/NBodySimulator.h` | Humano (`Requiere intervención humana: ...`) | Amaru Monje agregó el comentario `/** @file ... */` correspondiente | Corregido y fusionado vía PR #19 (commit `3ecf7b7`) |

*Nota: el documentador creó los 4 issues de este lote (#10, #11, #12 y #13)
en una misma ejecución
(`https://github.com/NicoM04/Lab1-Distri/actions/runs/30509741278`); se
documentan aquí los dos mínimos requeridos (#10 y #13). Correspondencia
issue↔archivo confirmada manualmente por el equipo:*

| Issue | Archivo |
|---|---|
| #10 | `nbody_2d/CudaBuffer.h` |
| #11 | `nbody_2d/CudaCheck.cuh` |
| #12 | `nbody_2d/NBodySystem.h` |
| #13 | `nbody_2d/NBodySimulator.h` |

*Los 4 fueron atendidos por el mismo commit `3ecf7b7` / PR #19.*

## Agente revisor de bugs

El revisor de bugs detectó, con `find_unchecked_cuda_calls`
(`scripts/agents/bug_reviewer.py`), llamadas a la API de CUDA
(`cudaDeviceSynchronize`, `cudaFree`) sin pasar por la macro `CUDA_CHECK`.
Ese hallazgo se corrigió en el commit `fbabdee` ("fix(cuda): falta de
CUDACHECK, bugs de github actions"), que envuelve en `CUDA_CHECK(...)` las
llamadas en `nbody_2d/NBodySimulator.cpp:187` y `:207`, y
`nbody_2d/kernels/metrics.cu:132` y `:165` — exactamente las 4 llamadas que
la heurística del agente señala. Ese commit se fusionó a `main` mediante el
**PR #18** (rama `fix/bug-resolve`, merge commit `8c15679`).

| Fecha | Issue | URL del issue | URL de ejecución (Actions) | Hallazgo / análisis producido | Clasificación | Acción humana posterior | Resultado final |
|---|---|---|---|---|---|---|---|
| 2026-07-30 o antes (resuelto por el commit `fbabdee`, 2026-07-30 16:34 -04:00) | #14 | https://github.com/NicoM04/Lab1-Distri/issues/14 | https://github.com/NicoM04/Lab1-Distri/actions/runs/30524015875 | `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**` — llamada `cudaDeviceSynchronize` sin `CUDA_CHECK` en `nbody_2d/NBodySimulator.cpp:187` | Mecánico (con sugerencia de parche `CUDA_CHECK(...)` en el cuerpo del issue) | Amaru Monje envolvió la llamada correspondiente en `CUDA_CHECK(...)` | Corregido y fusionado vía PR #18 (commit `fbabdee`) |
| 2026-07-30 o antes (resuelto por el commit `fbabdee`, 2026-07-30 16:34 -04:00) | #17 | https://github.com/NicoM04/Lab1-Distri/issues/17 | https://github.com/NicoM04/Lab1-Distri/actions/runs/30524015875 | `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**` — llamada `cudaFree` sin `CUDA_CHECK` en `nbody_2d/kernels/metrics.cu:165` | Mecánico | Amaru Monje envolvió la llamada correspondiente en `CUDA_CHECK(...)` | Corregido y fusionado vía PR #18 (commit `fbabdee`) |

*Nota: el revisor de bugs creó los 4 issues de este lote (#14, #15, #16 y
#17) en una misma ejecución
(`https://github.com/NicoM04/Lab1-Distri/actions/runs/30524015875`); se
documentan aquí los dos mínimos requeridos (#14 y #17). Correspondencia
issue↔hallazgo confirmada manualmente por el equipo:*

| Issue | Ubicación |
|---|---|
| #14 | `nbody_2d/NBodySimulator.cpp:187` |
| #15 | `nbody_2d/NBodySimulator.cpp:207` |
| #16 | `nbody_2d/kernels/metrics.cu:132` |
| #17 | `nbody_2d/kernels/metrics.cu:165` |

*Los 4 fueron atendidos por el mismo commit `fbabdee` / PR #18.*

## Agente revisor de pull requests

El revisor de PR comentó automáticamente, tras la finalización de `ci.yml`,
en las PR #18, #19, #21 y #23. Se documentan aquí las dos mínimas
requeridas (#18 y #23), que ilustran los dos motivos distintos por los que
el agente clasifica un cambio como "Requiere revisión humana" según
`classify_change` (`scripts/agents/pr_reviewer.py`):

| Fecha (merge) | PR | URL del PR | URL de ejecución (Actions) | Hallazgo / análisis producido | Clasificación | Acción humana posterior | Resultado final |
|---|---|---|---|---|---|---|---|
| 2026-07-30 18:34 -04:00 | #18 | https://github.com/NicoM04/Lab1-Distri/pull/18 | https://github.com/NicoM04/Lab1-Distri/actions/runs/30580028842 | `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**` — el diff modifica `nbody_2d/NBodySimulator.cpp` y `nbody_2d/kernels/metrics.cu`, archivos de física/kernels/memoria CUDA | Requiere revisión humana (coincide con `PHYSICS_SENSITIVE_RE`: toca kernels/física) | Revisión y aprobación humana antes de fusionar (rama `fix/bug-resolve`) | PR fusionada por una persona (merge commit `8c15679`); el agente no aprobó ni fusionó |
| 2026-07-30 22:00 -04:00 | #23 | https://github.com/NicoM04/Lab1-Distri/pull/23 | https://github.com/NicoM04/Lab1-Distri/actions/runs/30597706316 | `**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**` — el diff modifica archivos `.cpp`/`.py` (`BenchmarkCuda.cpp`, `lab2_plots/plot_real_data.py`), fuera de la lista de extensiones mecánicas permitidas (`.md/.yml/.yaml/.txt/.gitignore`) | Requiere revisión humana (no son archivos de documentación/formato/configuración evidente) | Revisión y aprobación humana antes de fusionar (rama `feature/plots`) | PR fusionada por una persona (merge commit `7b94574`); el agente no aprobó ni fusionó |

*Nota: las PR #19 y #21 también recibieron comentario automático del agente
en el mismo período; no se listan aquí por ser redundantes con el mínimo
pedido (#18 y #23). En los cuatro casos, el comentario del agente recordó
explícitamente que el merge lo realiza una persona humana — ningún agente
aprobó ni fusionó ninguna PR (verificado también en el código: no existe
ninguna llamada a la API de "merge" o "review/approve" en
`scripts/agents/pr_reviewer.py` ni en `common.py`).*

## Resumen de lo que confirman estas evidencias

- El documentador detectó **encabezados sin comentario introductorio** en
  archivos CUDA/host reales del proyecto.
- El revisor de bugs detectó **llamadas CUDA sin `CUDA_CHECK`** en archivos
  reales del proyecto.
- El revisor de PR clasificó como **"Requiere revisión humana"** tanto
  cambios que tocaban física/kernels/memoria CUDA (PR #18) como cambios
  fuera del alcance mecánico permitido (PR #23).
- Las tres ejecuciones observadas usaron el **fallback de análisis estático**
  (sin modelo de IA configurado), no un modelo generativo — el fallback
  estático **no equivale** a un modelo generativo de IA.
- Los issues y comentarios fueron **creados realmente por `github-actions`**
  dentro de ejecuciones reales de GitHub Actions, no simulados.
- **Ningún agente aprobó, fusionó ni hizo push automáticamente**; todas las
  aprobaciones y fusiones fueron acciones humanas (`8c15679`, `01ea9a1`,
  `34dab19`, `7b94574`).

## Estado de esta evidencia

Las seis evidencias mínimas (dos por agente) están **completas**: las seis
URLs de ejecución de GitHub Actions fueron verificadas manualmente por el
equipo y quedaron registradas en las tablas de arriba, y la correspondencia
issue↔archivo/línea dentro de cada lote (#10-#13 y #14-#17) también quedó
confirmada. Las seis URLs requeridas ya fueron registradas y no quedan marcadores pendientes.

Lo que sigue sin verificarse desde este entorno local (no bloquea el
cumplimiento del mínimo de dos evidencias por agente):

- El contenido textual completo de cada issue/comentario tal como quedó
  publicado en GitHub (aquí solo se cita el encabezado literal del
  fallback estático y el hallazgo asociado).
- Si además de las PR #18 y #23 documentadas, los comentarios en las PR
  #19 y #21 (mencionados arriba) contienen algún detalle adicional
  relevante.

## Cómo completar esta tabla

1. Ejecutar el agente (manualmente con `workflow_dispatch` o esperando su
   disparador programado).
2. Copiar la URL de la ejecución desde la pestaña **Actions** de GitHub
   (`https://github.com/<owner>/<repo>/actions/runs/<run_id>`).
3. Si el agente abrió un issue o comentó una PR, copiar su URL directa.
4. Completar "Resultado" con una descripción breve y verificable (p. ej.
   "Abrió issue #12 por enlace roto en README" o "Comentó PR #15: cambio
   mecánico, CI verde").
5. Indicar si un humano revisó el hallazgo y qué se decidió.
