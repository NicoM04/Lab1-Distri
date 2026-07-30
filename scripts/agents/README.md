# Agentes de IA — Rol 4 (Lab 2)

Tres scripts Python independientes, sin dependencias externas (solo
librería estándar), que implementan los agentes descritos en el issue #7:

| Script | Agente | Qué hace |
|---|---|---|
| `documenter.py` | Documentador | Revisa README(s) y `CHANGELOG.md`, abre issues por documentación faltante/rota. |
| `bug_reviewer.py` | Revisor de bugs | Analiza `main` en busca de señales mecánicas de riesgo (CUDA sin `CUDA_CHECK`, archivos generados versionados, tolerancias inconsistentes, y si la última ejecución de `ci.yml` en `main` falló — reutilizando ese resultado vía la API de Actions, sin re-ejecutar la suite). |
| `pr_reviewer.py` | Revisor de PR | Comenta una PR después de que su CI termina, clasificando el cambio. |

`common.py` centraliza la llamada al proveedor de IA y las llamadas mínimas
a la API REST de GitHub (issues, PRs, comentarios). Ver el docstring de ese
archivo para el detalle del proveedor de IA soportado.

## Requisitos

- Python 3.9 o superior (solo librería estándar: `argparse`, `json`,
  `urllib`, `re`, `subprocess`, `datetime`). No hay `requirements.txt`
  porque no se usan paquetes de terceros.
- `git` disponible en el `PATH` (usado por `bug_reviewer.py` para listar
  archivos versionados).

## Variables de entorno

| Variable | Uso | Obligatoria |
|---|---|---|
| `GITHUB_TOKEN` | Autenticación contra la API de GitHub (issues, PRs, comentarios) y, si no hay proveedor genérico, como credencial de GitHub Models. | Sí, salvo `--dry-run` |
| `GITHUB_REPOSITORY` | `owner/repo` objetivo. La define GitHub Actions automáticamente. | Sí, salvo `--dry-run` sin llamadas a la API |
| `AGENT_API_URL` | URL completa de un endpoint de chat completions compatible con OpenAI. | No (alternativa a GitHub Models) |
| `AGENT_API_KEY` | Credencial para `AGENT_API_URL`. | No |
| `AGENT_MODEL` | Nombre del modelo para `AGENT_API_URL`. | No |
| `GITHUB_MODELS_MODEL` | Modelo a usar en GitHub Models (default `openai/gpt-4o-mini`). | No |
| `AGENT_DRY_RUN` | Si es `1`/`true`/`yes`, equivalente a pasar `--dry-run`. | No |

**Ningún secreto se imprime en logs.** Los scripts solo leen tokens desde
variables de entorno y nunca los incluyen en los mensajes que imprimen o en
los cuerpos de issues/comentarios.

Si no hay ningún proveedor de IA configurado (`AGENT_API_URL`+`AGENT_API_KEY`
ni `GITHUB_TOKEN` utilizable como GitHub Models), los scripts **no simulan**
una respuesta: usan un cuerpo de hallazgo generado por el análisis estático,
claramente etiquetado como `ANÁLISIS ESTÁTICO (sin modelo de IA disponible)`,
o fallan con un mensaje explícito si además falta `GITHUB_TOKEN` fuera de
`--dry-run`.

## Ejecución manual

Desde la raíz del repositorio:

```bash
export GITHUB_TOKEN=...      # o usar --dry-run para no requerirlo
export GITHUB_REPOSITORY=NicoM04/Lab1-Distri

python scripts/agents/documenter.py --dry-run
python scripts/agents/bug_reviewer.py --dry-run
python scripts/agents/pr_reviewer.py --pr-number 12 --sha <sha> --conclusion success --dry-run
```

`--dry-run` (o `AGENT_DRY_RUN=1`) hace que ningún script escriba en GitHub:
solo imprime en stdout lo que habría creado o comentado.

## Garantías de diseño

- Ningún script hace `merge`, aprueba PRs ni escribe en `main`.
- Los issues usan un marcador HTML identificable
  (`<!-- agent:<nombre>:<key> -->`) para no duplicar el mismo hallazgo. La
  deduplicación y el conteo semanal consultan la etiqueta **específica del
  agente** (`agent-documenter`/`agent-bug-reviewer`), no una etiqueta
  genérica compartida como `bug`/`documentation` que también podrían usar
  issues creados manualmente por personas.
- `pr_reviewer.py` usa el SHA del commit como parte del marcador, así que
  nunca comenta dos veces lo mismo para el mismo commit (omite en vez de
  duplicar).
- `create_issue` en `common.py` respeta un tope de **5 issues automáticos
  por agente en los últimos 7 días**; si se alcanza, no crea más y lo indica
  explícitamente en el log. El límite es **fail-closed**: si no se puede
  verificar el conteo (error de red/API), tampoco se crea el issue.
- Si `create_issue` falla por cualquier motivo (etiqueta inexistente, error
  de red, etc.), `documenter.py`/`bug_reviewer.py` capturan el error,
  registran un mensaje claro y **continúan con el siguiente hallazgo** en
  vez de abortar toda la ejecución.
- Cuando un hallazgo/comentario se clasifica como no mecánico, la frase
  `Requiere intervención humana: <motivo>` (y, en el revisor de PR, el
  recordatorio de que el merge es humano) se **garantiza por código**
  (`common.ensure_human_notice`) — nunca se depende solo de que el modelo de
  IA haya seguido el prompt.
- `pr_reviewer.py` solo clasifica un cambio como mecánico si la PR vincula
  un issue con una palabra clave de cierre reconocida por GitHub
  (`Closes #7`, `Fixes #7`, `Resolves #7`, `Refs #7`, etc., insensible a
  mayúsculas); no valida si ese issue existe realmente, solo la sintaxis.
- Los permisos de GitHub Actions se declaran mínimos en cada workflow (ver
  `.github/workflows/agent-*.yml`); `bug_reviewer.py` requiere además
  `actions: read` para consultar (sin re-ejecutar) la última conclusión de
  `ci.yml`.
- **Limitación conocida:** el chequeo de CI del revisor de bugs cubre la
  suite CPU (Catch2) que `ci.yml` ejecuta vía `make test`. Los tests CUDA
  solo se compilan en CI (no hay GPU en el runner de Actions), por lo que
  su ejecución no está cubierta, y el agente no detecta regresiones
  semánticas de física.
