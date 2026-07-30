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
  `feature/git-releases-agents`).
- `fix/<descripcion-corta>`: corrección de errores.
- Nombre descriptivo en kebab-case, sin incluir el número de issue en el
  nombre de la rama (el vínculo se hace en la PR, no en el nombre).

## 3. Ciclo de vida de un cambio

1. Crear una rama `feature/*` o `fix/*` desde `main` actualizada.
2. Trabajar y commitear en esa rama (nunca en `main`).
3. Abrir una PR hacia `main` que incluya `Closes #<issue>`.
4. Esperar a que `ci.yml` termine en verde.
5. Solicitar revisión humana (al menos un/a integrante del equipo, o el
   docente/ayudante según corresponda).
6. Aplicar los cambios solicitados en la misma rama.
7. Una vez aprobada y con CI en verde, una persona humana fusiona la PR
   (merge o squash, según convenga).
8. Eliminar la rama fusionada (localmente y en el remoto) para mantener el
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
