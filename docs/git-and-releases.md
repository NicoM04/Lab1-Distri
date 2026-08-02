# Flujo Git y procedimiento de releases (Rol 4 — Lab 2)

Este documento describe el flujo de trabajo Git y el procedimiento de
releases acordado para el Laboratorio 2. Complementa la tabla resumen del
`README.md` principal.

## 1. Estado de `main`

- `main` está **protegida**: no se permite push directo ni commits directos.
- Todo cambio llega a `main` mediante una Pull Request (PR).
- La PR debe:
  - estar vinculada a un issue (usar `Closes #<numero>` en la descripción);
  - tener el pipeline de CI (`ci.yml`) en verde;
  - pasar por revisión humana antes de fusionarse.
- Ningún agente de IA puede aprobar, fusionar ni cerrar una PR. Los agentes
  solo leen, comentan y —cuando corresponde— abren issues.

## 2. Ramas

- `feature/<descripcion-corta>`: nueva funcionalidad (p. ej.
  `feature/git-releases-agents`, `feature/plots`).
- `fix/<descripcion-corta>`: corrección de errores (p. ej.
  `fix/bug-resolve`).
- `docs/<descripcion-corta>`: cambios que son solo de documentación, sin
  tocar código (p. ej. `docs/code-comments`, `docs/finalize-role4`).
- Nombre descriptivo en kebab-case, sin incluir el número de issue en el
  nombre de la rama (el vínculo se hace en la PR, no en el nombre).

## 3. Ciclo de vida de un cambio

El flujo real, ya observado en el historial de `main`, es:

**issue → rama `feature/`/`fix/`/`docs/` → commits → PR → CI
(`ci.yml`) → comentario automático del agente revisor de PR → revisión
humana → aprobación humana → merge.**

En detalle:

1. Crear una rama `feature/*`, `fix/*` o `docs/*` desde `main` actualizada.
2. Trabajar y commitear en esa rama (nunca en `main`).
3. Abrir una PR hacia `main` que incluya `Closes #<issue>`.
4. Esperar a que `ci.yml` termine (en verde o en fallo).
5. El workflow `agent-pr-reviewer.yml` se dispara automáticamente al
   terminar `ci.yml` (evento `workflow_run`) y publica un comentario en la
   PR indicando el resultado del CI y si el cambio parece mecánico o
   requiere revisión humana. Este comentario **nunca aprueba ni fusiona**
   la PR; es solo informativo (ver `docs/agents-evidence.md` para ejemplos
   reales de este comportamiento, p. ej. PR #18 y #23).
6. Solicitar y obtener revisión humana (al menos un/a integrante del
   equipo, o el docente/ayudante según corresponda).
7. Aplicar los cambios solicitados en la misma rama.
8. Una vez aprobada por una persona y con CI en verde, esa persona humana
   fusiona la PR (merge o squash, según convenga). El merge es siempre una
   acción humana, nunca del agente.
9. Eliminar la rama fusionada (localmente y en el remoto) para mantener el
   repositorio limpio.

## 4. Rol de los agentes de IA en este flujo

Los tres agentes (documentador, revisor de bugs, revisor de PR) son
**solo lectura respecto a `main`**: no hacen push a `main`, no fusionan PRs y
no aprueban revisiones. Su única escritura permitida es:

- abrir issues etiquetados (documentador, revisor de bugs);
- comentar en una PR ya existente (revisor de PR).

Ver la tabla de agentes en el `README.md` principal para el detalle de
disparadores, entradas/salidas y permisos de cada uno.

## 5. Procedimiento de release `v2.0.0-lab2`

**Importante: esta release todavía NO se ha publicado.** Los pasos abajo son
el procedimiento a seguir cuando el equipo decida publicarla; no deben
ejecutarse como parte de este trabajo.

El tag y la release:

- solo deben crearse desde `main` ya actualizada (con `CHANGELOG.md`
  movido a `[2.0.0]` y todo lo demás fusionado);
- deben crearse **después** de fusionar todos los entregables pendientes
  (incluyendo el cierre documental del Rol 4);
- **no se crean en esta rama** (`docs/finalize-role4` es solo
  documentación; el tag se crea desde `main` una vez que esta rama, u otra
  equivalente, ya esté fusionada).

1. Actualizar `CHANGELOG.md`: mover las entradas de `[Unreleased]` a una
   nueva sección `## [2.0.0] - YYYY-MM-DD` con la fecha real de publicación.
2. Confirmar que `ci.yml` está en verde sobre `main`.
3. Fusionar cualquier PR pendiente que deba incluirse en la release.
4. Crear el tag `v2.0.0-lab2` desde `main` (`git tag -a v2.0.0-lab2 -m "..."`
   y `git push origin v2.0.0-lab2`), solo después de los pasos anteriores.
5. Publicar las notas de la release en GitHub usando como base
   `docs/release-notes-v2.0.0-lab2.md` (retirando la marca de BORRADOR una
   vez verificado su contenido).

## 6. Buenas prácticas de commits

- Mensajes en modo imperativo, idealmente con prefijo de tipo
  (`feat`, `fix`, `docs`, `chore`, `test`, `build`), como ya se observa en el
  historial (`feat(cuda): ...`, `docs(cuda): ...`, `test(cuda): ...`).
- Evitar commits genéricos tipo "fix" repetidos cuando sea posible; si el CI
  falla varias veces, se prefiere iterar en la misma PR antes de fusionar.
