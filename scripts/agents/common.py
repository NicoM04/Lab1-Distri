"""Utilidades compartidas por los agentes de IA del Rol 4 (Lab 2).

Este módulo centraliza:

- la llamada al proveedor de IA (agnóstico de proveedor, ver `call_model`);
- las llamadas mínimas a la API REST de GitHub necesarias para leer
  issues/PRs y publicar issues/comentarios;
- utilidades de marcadores para evitar issues/comentarios duplicados;
- un contador simple para respetar el límite de issues automáticos
  semanales, contado por la etiqueta específica de cada agente
  (`agent-<nombre>`), en modo fail-closed: si no se puede verificar el
  conteo, no se crea el issue;
- `ensure_human_notice`, que garantiza por código (no solo por prompt) que
  un hallazgo no mecánico incluya la frase "Requiere intervención humana:
  <motivo>".

Ningún agente que use este módulo hace merge, aprueba PRs ni escribe en
`main`. El token de GitHub se lee únicamente desde la variable de entorno
`GITHUB_TOKEN` (nunca se imprime ni se hardcodea).

Proveedor de IA (en orden de prioridad):

1. Interfaz genérica configurable vía variables de entorno:
   - `AGENT_API_URL`: URL completa del endpoint de chat completions,
     compatible con el formato de OpenAI
     (`{"model": ..., "messages": [...]}` -> `choices[0].message.content`).
   - `AGENT_API_KEY`: credencial enviada como `Authorization: Bearer <key>`.
   - `AGENT_MODEL`: nombre del modelo a solicitar.
2. GitHub Models, usando el `GITHUB_TOKEN` de Actions como credencial
   (requiere que el repositorio/organización tenga GitHub Models
   habilitado y que el workflow declare permiso `models: read`).

Si ninguna está disponible, `call_model` lanza `AgentError` con un mensaje
claro. Nunca se simula una respuesta del modelo.
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import re
import socket
import sys
import urllib.error
import urllib.request

GITHUB_API_ROOT = "https://api.github.com"
GITHUB_MODELS_URL = "https://models.github.ai/inference/chat/completions"
DEFAULT_GITHUB_MODEL = "openai/gpt-4o-mini"

# Límite de issues automáticos por agente sin revisión humana, ver enunciado
# del Rol 4 ("máximo 5 issues automáticos abiertos por agente durante una
# semana sin revisión humana").
MAX_AUTO_ISSUES_PER_WEEK = 5

MARKER_RE = re.compile(r"<!--\s*agent:(?P<agent>[a-zA-Z0-9_-]+):(?P<key>[a-zA-Z0-9_.-]+)\s*-->")


class AgentError(RuntimeError):
    """Error claro y accionable para que el workflow falle o degrade a dry-run."""


def log(message: str) -> None:
    print(f"[agents] {message}", flush=True)


# ---------------------------------------------------------------------------
# Marcadores para evitar duplicados
# ---------------------------------------------------------------------------

def make_marker(agent: str, key: str) -> str:
    """Genera un marcador HTML identificable, p. ej. <!-- agent:documenter:readme-toc -->."""
    safe_key = re.sub(r"[^a-zA-Z0-9_.-]", "-", key)
    return f"<!-- agent:{agent}:{safe_key} -->"


def find_marker_keys(text: str, agent: str) -> set:
    """Devuelve el conjunto de keys ya marcadas por `agent` dentro de `text`."""
    return {m.group("key") for m in MARKER_RE.finditer(text or "") if m.group("agent") == agent}


# ---------------------------------------------------------------------------
# Garantía de intervención humana (nunca depender solo del prompt)
# ---------------------------------------------------------------------------

HUMAN_NOTICE_PHRASE = "Requiere intervención humana"


def ensure_human_notice(text: str, reason: str) -> str:
    """Garantiza por código que un texto no mecánico incluya la frase exigida.

    No confía en que el modelo de IA haya seguido el prompt: si la frase no
    está presente (insensible a mayúsculas), se antepone una explicación
    generada por código, sin descartar el texto original.
    """
    if HUMAN_NOTICE_PHRASE.lower() in (text or "").lower():
        return text
    return f"**{HUMAN_NOTICE_PHRASE}: {reason}**\n\n{text or ''}"


# ---------------------------------------------------------------------------
# Configuración
# ---------------------------------------------------------------------------

class Config:
    def __init__(self, dry_run: bool = False):
        self.github_token = os.environ.get("GITHUB_TOKEN", "")
        self.repo = os.environ.get("GITHUB_REPOSITORY", "")
        self.agent_api_url = os.environ.get("AGENT_API_URL", "")
        self.agent_api_key = os.environ.get("AGENT_API_KEY", "")
        self.agent_model = os.environ.get("AGENT_MODEL", "")
        self.github_models_model = os.environ.get("GITHUB_MODELS_MODEL", DEFAULT_GITHUB_MODEL)
        self.dry_run = dry_run or os.environ.get("AGENT_DRY_RUN", "").strip().lower() in (
            "1",
            "true",
            "yes",
        )

    def has_generic_provider(self) -> bool:
        return bool(self.agent_api_url and self.agent_api_key)

    def has_github_models_provider(self) -> bool:
        return bool(self.github_token)

    def has_model_provider(self) -> bool:
        return self.has_generic_provider() or self.has_github_models_provider()

    def require_repo(self) -> str:
        if not self.repo:
            raise AgentError(
                "GITHUB_REPOSITORY no está definido. Este script debe ejecutarse "
                "dentro de GitHub Actions, o exportar GITHUB_REPOSITORY=owner/repo "
                "manualmente para pruebas locales."
            )
        return self.repo


def build_arg_parser(description: str) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="No escribe en GitHub (no crea issues ni comentarios); solo imprime lo que haría.",
    )
    return parser


# ---------------------------------------------------------------------------
# Llamadas HTTP mínimas (stdlib únicamente, sin dependencias externas)
# ---------------------------------------------------------------------------

def _urlopen(method: str, url: str, headers: dict, data: bytes = None, timeout: int = 30):
    """Punto único de acceso HTTP: toda excepción de red/timeout se traduce a AgentError.

    Nunca incluye headers (y por lo tanto nunca el token) en el mensaje de error.
    """
    req = urllib.request.Request(url, data=data, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise AgentError(f"HTTP {exc.code} en {method} {url}: {body[:500]}") from None
    except urllib.error.URLError as exc:
        raise AgentError(f"No se pudo conectar a {url}: {exc.reason}") from None
    except (socket.timeout, TimeoutError) as exc:
        raise AgentError(f"Tiempo de espera agotado llamando a {method} {url}: {exc}") from None


def _http_json(method: str, url: str, headers: dict, payload: dict = None, timeout: int = 30):
    data = json.dumps(payload).encode("utf-8") if payload is not None else None
    status, body = _urlopen(method, url, headers, data=data, timeout=timeout)
    try:
        return status, (json.loads(body) if body else None)
    except json.JSONDecodeError as exc:
        raise AgentError(f"Respuesta no es JSON válido desde {method} {url}: {exc}") from None


def _http_text(method: str, url: str, headers: dict, timeout: int = 30) -> str:
    _, body = _urlopen(method, url, headers, timeout=timeout)
    return body


def gh_headers(token: str, accept: str = "application/vnd.github+json") -> dict:
    return {
        "Authorization": f"Bearer {token}",
        "Accept": accept,
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "lab2-rol4-agents",
    }


def gh_get(path: str, config: Config, accept: str = "application/vnd.github+json"):
    if not config.github_token:
        raise AgentError("GITHUB_TOKEN no está definido; no se puede leer la API de GitHub.")
    url = f"{GITHUB_API_ROOT}{path}"
    _, body = _http_json("GET", url, gh_headers(config.github_token, accept))
    return body


def gh_post(path: str, config: Config, payload: dict):
    if not config.github_token:
        raise AgentError("GITHUB_TOKEN no está definido; no se puede escribir en la API de GitHub.")
    url = f"{GITHUB_API_ROOT}{path}"
    _, body = _http_json("POST", url, gh_headers(config.github_token), payload)
    return body


def gh_patch(path: str, config: Config, payload: dict):
    if not config.github_token:
        raise AgentError("GITHUB_TOKEN no está definido; no se puede escribir en la API de GitHub.")
    url = f"{GITHUB_API_ROOT}{path}"
    _, body = _http_json("PATCH", url, gh_headers(config.github_token), payload)
    return body


# ---------------------------------------------------------------------------
# Issues
# ---------------------------------------------------------------------------

def list_issues(config: Config, label: str, state: str = "all", per_page: int = 100) -> list:
    repo = config.require_repo()
    path = f"/repos/{repo}/issues?labels={label}&state={state}&per_page={per_page}"
    result = gh_get(path, config)
    return [i for i in (result or []) if "pull_request" not in i]


def count_recent_label_issues(config: Config, label: str, since_days: int = 7) -> int:
    """Cuenta issues con `label` creados en los últimos `since_days` días (para el tope semanal)."""
    issues = list_issues(config, label, state="all")
    cutoff = datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(days=since_days)
    count = 0
    for issue in issues:
        created_at = issue.get("created_at", "")
        try:
            created = datetime.datetime.strptime(created_at, "%Y-%m-%dT%H:%M:%SZ").replace(
                tzinfo=datetime.timezone.utc
            )
        except ValueError:
            continue
        if created >= cutoff:
            count += 1
    return count


def issue_marker_exists(config: Config, label: str, agent: str, key: str) -> bool:
    for issue in list_issues(config, label, state="all"):
        if key in find_marker_keys(issue.get("body", ""), agent):
            return True
    return False


def create_issue(
    config: Config,
    title: str,
    body: str,
    labels: list,
    agent: str,
) -> dict:
    """Crea un issue respetando el límite semanal y el modo --dry-run.

    El conteo semanal se hace por la etiqueta específica del agente
    (`agent-<agent>`), no por una etiqueta genérica compartida como
    `bug`/`documentation`, para no contar issues humanos ajenos al bot.

    Si no se puede verificar el límite semanal (error de red/API), la
    política es fail-closed: no se crea el issue. Nunca se asume `recent = 0`.

    Devuelve el issue creado, o `None` si se omitió (dry-run, límite
    alcanzado, o fallo al verificar el límite).
    """
    agent_label = f"agent-{agent}"
    try:
        recent = count_recent_label_issues(config, agent_label)
    except AgentError as exc:
        log(
            "No se pudo verificar el límite semanal; no se creará el issue. "
            f"Requiere intervención humana: revisar la API de GitHub ({exc})."
        )
        return None

    if recent >= MAX_AUTO_ISSUES_PER_WEEK:
        log(
            f"Límite semanal de {MAX_AUTO_ISSUES_PER_WEEK} issues automáticos para "
            f"'{agent_label}' alcanzado ({recent} en los últimos 7 días). No se crea un nuevo issue; "
            "se requiere revisión humana antes de continuar."
        )
        return None

    if config.dry_run:
        log(f"[dry-run] Se crearía un issue: título={title!r} labels={labels}")
        log(f"[dry-run] Cuerpo:\n{body}")
        return None

    repo = config.require_repo()
    payload = {"title": title, "body": body, "labels": labels}
    issue = gh_post(f"/repos/{repo}/issues", config, payload)
    log(f"Issue creado: {issue.get('html_url')}")
    return issue


# ---------------------------------------------------------------------------
# Pull requests
# ---------------------------------------------------------------------------

def get_pulls_for_commit(config: Config, sha: str) -> list:
    repo = config.require_repo()
    return gh_get(f"/repos/{repo}/commits/{sha}/pulls", config) or []


def get_pull(config: Config, number: int) -> dict:
    repo = config.require_repo()
    return gh_get(f"/repos/{repo}/pulls/{number}", config)


def get_pull_diff(config: Config, number: int) -> str:
    repo = config.require_repo()
    if not config.github_token:
        raise AgentError("GITHUB_TOKEN no está definido; no se puede leer el diff de la PR.")
    url = f"{GITHUB_API_ROOT}/repos/{repo}/pulls/{number}"
    headers = gh_headers(config.github_token, accept="application/vnd.github.v3.diff")
    return _http_text("GET", url, headers, timeout=30)


def list_pr_comments(config: Config, number: int) -> list:
    repo = config.require_repo()
    return gh_get(f"/repos/{repo}/issues/{number}/comments?per_page=100", config) or []


def create_or_update_pr_comment(
    config: Config,
    number: int,
    body: str,
    agent: str,
    key: str,
) -> dict:
    """Publica un comentario en la PR, o lo actualiza si ya existe uno con el mismo marcador.

    Esto evita comentar dos veces lo mismo para el mismo commit (el `key`
    debe incluir el SHA revisado).
    """
    marker = make_marker(agent, key)
    full_body = f"{body}\n\n{marker}"

    existing = None
    for comment in list_pr_comments(config, number):
        if key in find_marker_keys(comment.get("body", ""), agent):
            existing = comment
            break

    if config.dry_run:
        action = "actualizaría" if existing else "crearía"
        log(f"[dry-run] Se {action} un comentario en PR #{number} con marcador {marker}")
        log(f"[dry-run] Cuerpo:\n{full_body}")
        return None

    repo = config.require_repo()
    if existing:
        log(f"Ya existe un comentario para {marker} en PR #{number}; se omite duplicado.")
        return existing
    comment = gh_post(f"/repos/{repo}/issues/{number}/comments", config, {"body": full_body})
    log(f"Comentario publicado: {comment.get('html_url')}")
    return comment


def get_workflow_run(config: Config, run_id: int) -> dict:
    repo = config.require_repo()
    return gh_get(f"/repos/{repo}/actions/runs/{run_id}", config)


def get_latest_workflow_run(config: Config, workflow_file: str, branch: str = "main") -> dict:
    """Devuelve la última ejecución *completada* de `workflow_file` sobre `branch`, o None.

    Reutiliza el resultado ya calculado por un workflow existente (p. ej.
    `ci.yml`) en vez de re-ejecutar su lógica. Requiere el permiso
    `actions: read` en el workflow que llama a esta función.
    """
    repo = config.require_repo()
    path = f"/repos/{repo}/actions/workflows/{workflow_file}/runs?branch={branch}&status=completed&per_page=1"
    result = gh_get(path, config)
    runs = (result or {}).get("workflow_runs") or []
    return runs[0] if runs else None


# ---------------------------------------------------------------------------
# Proveedor de IA (agnóstico)
# ---------------------------------------------------------------------------

def call_model(system_prompt: str, user_prompt: str, config: Config, timeout: int = 60) -> str:
    """Llama al proveedor de IA configurado y devuelve el texto de la respuesta.

    Lanza `AgentError` si no hay ningún proveedor configurado. Nunca
    devuelve una respuesta simulada.
    """
    if config.has_generic_provider():
        return _call_openai_compatible(
            url=config.agent_api_url,
            api_key=config.agent_api_key,
            model=config.agent_model or "default",
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            timeout=timeout,
        )

    if config.has_github_models_provider():
        return _call_openai_compatible(
            url=GITHUB_MODELS_URL,
            api_key=config.github_token,
            model=config.github_models_model,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            timeout=timeout,
        )

    raise AgentError(
        "No hay proveedor de IA configurado. Define AGENT_API_URL + AGENT_API_KEY "
        "(y opcionalmente AGENT_MODEL), o ejecuta dentro de GitHub Actions con "
        "GITHUB_TOKEN y permiso 'models: read' para usar GitHub Models. "
        "No se simulará ninguna respuesta."
    )


def _call_openai_compatible(
    url: str, api_key: str, model: str, system_prompt: str, user_prompt: str, timeout: int
) -> str:
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
    }
    payload = {
        "model": model,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_prompt},
        ],
        "temperature": 0.2,
    }
    _, body = _http_json("POST", url, headers, payload, timeout=timeout)
    try:
        return body["choices"][0]["message"]["content"]
    except (KeyError, IndexError, TypeError) as exc:
        raise AgentError(f"Respuesta inesperada del proveedor de IA: {body}") from exc


# ---------------------------------------------------------------------------
# Entrada de conveniencia para probar la configuración sin escribir nada
# ---------------------------------------------------------------------------

def describe_provider(config: Config) -> str:
    if config.has_generic_provider():
        return f"interfaz genérica configurable (AGENT_API_URL={config.agent_api_url}, modelo={config.agent_model or 'default'})"
    if config.has_github_models_provider():
        return f"GitHub Models (modelo={config.github_models_model}) vía GITHUB_TOKEN"
    return "ninguno configurado"


if __name__ == "__main__":
    cfg = Config()
    log(f"Proveedor de IA detectado: {describe_provider(cfg)}")
    log(f"Modo dry-run: {cfg.dry_run}")
    sys.exit(0)
