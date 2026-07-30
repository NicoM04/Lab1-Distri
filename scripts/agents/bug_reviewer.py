#!/usr/bin/env python3
"""Agente revisor de bugs (Rol 4 — Lab 2).

Analiza el árbol de trabajo (pensado para ejecutarse sobre `main`) en busca
de señales mecánicas de riesgo:

- llamadas a la API de CUDA que no pasan por la macro `CUDA_CHECK`;
- archivos generados/binarios versionados por error (deberían estar en
  `.gitignore`);
- valores de tolerancia (`rtol`/`atol`) inconsistentes entre archivos de
  test;
- si la última ejecución completada de `ci.yml` sobre `main` falló.

Sobre este último punto: el chequeo **reutiliza el resultado ya calculado
por `ci.yml`** (vía la API de Actions) en vez de re-ejecutar la suite de
tests, para no duplicar la infraestructura de CI. `ci.yml` solo ejecuta
(`make test`) la suite CPU (Catch2); los tests CUDA se compilan pero no se
ejecutan en el runner (no hay GPU en GitHub Actions). Por lo tanto, este
agente **no detecta regresiones semánticas ni ejecuta/evalúa la suite
CUDA**; solo informa si el resultado ya público de CI (suite CPU) es
exitoso o no. Diagnosticar la causa de un test roto siempre se marca como
"Requiere intervención humana", porque el análisis estático no puede
determinarla.

No modifica `main`: solo abre issues (o sugiere un parche dentro del cuerpo
del issue, nunca aplicado automáticamente). Los hallazgos que toquen física,
la API pública, el orden del integrador, la lógica de kernels o la
estrategia de reducción/sincronización no trivial se marcan explícitamente
como "Requiere intervención humana".

Uso:
    python scripts/agents/bug_reviewer.py [--dry-run]
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common  # noqa: E402

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
AGENT_NAME = "bug-reviewer"
ISSUE_LABELS = ["bug", "agent-bug-reviewer"]
PROMPT_PATH = os.path.join(os.path.dirname(__file__), "prompts", "bug_reviewer.md")

SOURCE_EXTS = (".cu", ".cuh", ".cpp", ".h")
CUDA_CALL_RE = re.compile(r"\b(cudaMalloc|cudaFree|cudaMemcpy|cudaDeviceSynchronize|cudaGetLastError)\s*\(")
TOLERANCE_RE = re.compile(r"\b(rtol|atol)\b\s*=\s*([0-9.eE+-]+)")

GENERATED_PATTERNS = (
    re.compile(r"(^|/)nbody_2d$"),
    re.compile(r"\.o$"),
    re.compile(r"\.a$"),
    re.compile(r"\.so$"),
    re.compile(r"\.dat$"),
    re.compile(r"\.png$"),
    re.compile(r"\.csv$"),
    re.compile(r"(^|/)(run_tests|test_acceleration|gpu_accelerations_smoke_test|"
               r"cuda_buffer_roundtrip_test|gpu_integration_test|gpu_energy_test)$"),
)

EXPECTED_TOLERANCES = {"rtol": "1e-4", "atol": "1e-8"}

# Workflow de CI cuyo resultado se reutiliza (ver docstring del módulo):
# no se re-ejecuta la suite de tests, solo se consulta su última conclusión.
CI_WORKFLOW_FILE = "ci.yml"
CI_BRANCH = "main"


def load_system_prompt() -> str:
    with open(PROMPT_PATH, "r", encoding="utf-8") as fh:
        return fh.read()


def run_git(args: list) -> str:
    result = subprocess.run(
        ["git", *args], cwd=REPO_ROOT, capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        raise common.AgentError(f"git {' '.join(args)} falló: {result.stderr.strip()}")
    return result.stdout


def iter_source_files() -> list:
    files = []
    nbody_dir = os.path.join(REPO_ROOT, "nbody_2d")
    for root, _dirs, filenames in os.walk(nbody_dir):
        for name in filenames:
            if name.endswith(SOURCE_EXTS):
                files.append(os.path.join(root, name))
    return files


def _strip_comments(line: str, in_block_comment: bool) -> tuple:
    """Elimina comentarios `//` y `/* ... */` de una línea (heurística simple, no un parser real)."""
    if in_block_comment:
        end = line.find("*/")
        if end == -1:
            return "", True
        line = line[end + 2:]
        in_block_comment = False

    while True:
        start = line.find("/*")
        if start == -1:
            break
        end = line.find("*/", start + 2)
        if end == -1:
            return line[:start], True
        line = line[:start] + line[end + 2:]

    return line.split("//", 1)[0], in_block_comment


def find_unchecked_cuda_calls() -> list:
    findings = []
    for path in iter_source_files():
        rel = os.path.relpath(path, REPO_ROOT)
        in_block_comment = False
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for line_no, raw_line in enumerate(fh, start=1):
                code_part, in_block_comment = _strip_comments(raw_line, in_block_comment)
                stripped = code_part.strip()
                if not stripped or "CUDA_CHECK" in code_part:
                    continue
                match = CUDA_CALL_RE.search(code_part)
                if match:
                    findings.append((rel, line_no, stripped, match.group(1)))
    return findings


def find_tracked_generated_files() -> list:
    tracked = run_git(["ls-files"]).splitlines()
    return [p for p in tracked if any(pat.search(p) for pat in GENERATED_PATTERNS)]


def find_inconsistent_tolerances() -> list:
    findings = []
    tests_dir = os.path.join(REPO_ROOT, "nbody_2d", "tests")
    if not os.path.isdir(tests_dir):
        return findings
    for name in os.listdir(tests_dir):
        if not name.endswith((".cpp", ".cu")):
            continue
        path = os.path.join(tests_dir, name)
        rel = os.path.relpath(path, REPO_ROOT)
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for line_no, line in enumerate(fh, start=1):
                match = TOLERANCE_RE.search(line)
                if not match:
                    continue
                kind, value = match.group(1), match.group(2)
                expected = EXPECTED_TOLERANCES.get(kind)
                if expected and value != expected:
                    findings.append((rel, line_no, kind, value, expected))
    return findings


def check_ci_status(config: common.Config):
    """Consulta (sin re-ejecutar) la última ejecución completada de `ci.yml` en `main`.

    Devuelve el dict de la ejecución si su conclusión no fue "success", o
    `None` si fue exitosa, no se encontró ninguna, o no se pudo consultar
    (p. ej. falta el permiso `actions: read`).
    """
    try:
        run = common.get_latest_workflow_run(config, CI_WORKFLOW_FILE, CI_BRANCH)
    except common.AgentError as exc:
        common.log(f"No se pudo consultar el estado de CI en {CI_BRANCH}: {exc}")
        return None
    if run is None or run.get("conclusion") == "success":
        return None
    return run


def build_issue_body(
    system_prompt: str,
    config: common.Config,
    finding_desc: str,
    mechanical: bool,
    human_reason: str = "el hallazgo puede afectar física, sincronización no trivial o resultados numéricos",
) -> str:
    if config.has_model_provider():
        try:
            text = common.call_model(
                system_prompt,
                f"Señal detectada por análisis estático:\n\n{finding_desc}\n\n"
                f"Clasificación sugerida: {'Mecánico' if mechanical else 'Humano'}.",
                config,
            )
            # No confiar solo en que el modelo siguió el prompt: forzar la frase por código.
            if not mechanical:
                text = common.ensure_human_notice(text, human_reason)
            return text
        except common.AgentError as exc:
            common.log(f"Fallo al llamar al proveedor de IA, se usa cuerpo estático: {exc}")

    fallback = [
        "**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**",
        "",
        finding_desc,
        "",
        f"Clasificación preliminar: {'Mecánico' if mechanical else 'Humano'}.",
    ]
    if not mechanical:
        fallback.append(
            "Requiere intervención humana: el hallazgo puede afectar física, sincronización "
            "no trivial o resultados numéricos, y el análisis estático no puede evaluar ese impacto."
        )
    return "\n".join(fallback)


def main() -> int:
    parser = common.build_arg_parser("Agente revisor de bugs: analiza main en busca de riesgos mecánicos.")
    args = parser.parse_args()
    config = common.Config(dry_run=args.dry_run)

    common.log(f"Proveedor de IA: {common.describe_provider(config)}")
    if config.dry_run:
        common.log("Modo --dry-run activo: no se escribirá en GitHub.")
    elif not config.github_token:
        common.log("GITHUB_TOKEN no está definido y no se pasó --dry-run. Aborta.")
        return 1

    system_prompt = load_system_prompt()

    try:
        unchecked_calls = find_unchecked_cuda_calls()
    except OSError as exc:
        common.log(f"No se pudo escanear fuentes CUDA: {exc}")
        unchecked_calls = []

    try:
        tracked_generated = find_tracked_generated_files()
    except common.AgentError as exc:
        common.log(f"No se pudo consultar git ls-files: {exc}")
        tracked_generated = []

    tolerance_findings = find_inconsistent_tolerances()

    ci_run = None
    if config.github_token:
        ci_run = check_ci_status(config)
    else:
        common.log("Sin GITHUB_TOKEN no se puede consultar el estado de CI en main; se omite ese chequeo.")

    if not unchecked_calls and not tracked_generated and not tolerance_findings and not ci_run:
        common.log("Sin señales de riesgo en esta ejecución.")
        return 0

    for rel, line_no, line_text, call_name in unchecked_calls:
        key = f"unchecked-cuda-{rel}-{line_no}"
        title = f"bug: {call_name} sin CUDA_CHECK en {rel}:{line_no}"
        desc = (
            f"En `{rel}:{line_no}` se llama a `{call_name}(...)` sin pasar por la macro "
            f"`CUDA_CHECK`, por lo que un error de la API de CUDA podría pasar inadvertido.\n\n"
            f"Línea: `{line_text}`\n\n"
            f"Sugerencia (no aplicada automáticamente): envolver la llamada con `CUDA_CHECK(...)`, "
            "siguiendo el patrón ya usado en el resto del archivo."
        )
        if config.dry_run:
            body = build_issue_body(system_prompt, config, desc, mechanical=True)
            common.log(f"[dry-run] Hallazgo: {title}\n{body}")
            continue
        if common.issue_marker_exists(config, ISSUE_LABELS[1], AGENT_NAME, key):
            common.log(f"Ya existe un issue para {key}; se omite.")
            continue
        body = build_issue_body(system_prompt, config, desc, mechanical=True)
        body += f"\n\n{common.make_marker(AGENT_NAME, key)}"
        try:
            common.create_issue(config, title, body, ISSUE_LABELS, AGENT_NAME)
        except common.AgentError as exc:
            common.log(f"No se pudo crear el issue para {key}: {exc}. Se continúa con el siguiente hallazgo.")

    if tracked_generated:
        key = "tracked-generated-files"
        title = "bug: archivos generados versionados en el repositorio"
        listing = "\n".join(f"- `{p}`" for p in tracked_generated)
        desc = (
            "Los siguientes archivos parecen ser binarios/artefactos generados por la build, "
            f"pero están versionados en git:\n\n{listing}\n\n"
            "Sugerencia (no aplicada automáticamente): `git rm --cached <archivo>` y confirmar "
            "que el patrón correspondiente está en `.gitignore`."
        )
        if config.dry_run:
            body = build_issue_body(system_prompt, config, desc, mechanical=True)
            common.log(f"[dry-run] Hallazgo: {title}\n{body}")
        elif not common.issue_marker_exists(config, ISSUE_LABELS[1], AGENT_NAME, key):
            body = build_issue_body(system_prompt, config, desc, mechanical=True)
            body += f"\n\n{common.make_marker(AGENT_NAME, key)}"
            try:
                common.create_issue(config, title, body, ISSUE_LABELS, AGENT_NAME)
            except common.AgentError as exc:
                common.log(f"No se pudo crear el issue para {key}: {exc}. Se continúa con el siguiente hallazgo.")
        else:
            common.log(f"Ya existe un issue para {key}; se omite.")

    for rel, line_no, kind, value, expected in tolerance_findings:
        key = f"tolerance-{rel}-{line_no}"
        title = f"bug: {kind}={value} inconsistente con el resto de la suite en {rel}:{line_no}"
        desc = (
            f"En `{rel}:{line_no}` se usa `{kind} = {value}`, distinto del valor documentado "
            f"({kind} = {expected}) en el resto de las pruebas CPU/GPU. Esto podría ser una copia "
            "incorrecta de la tolerancia, o una decisión intencional no documentada."
        )
        if config.dry_run:
            body = build_issue_body(system_prompt, config, desc, mechanical=False)
            common.log(f"[dry-run] Hallazgo: {title}\n{body}")
            continue
        if common.issue_marker_exists(config, ISSUE_LABELS[1], AGENT_NAME, key):
            common.log(f"Ya existe un issue para {key}; se omite.")
            continue
        body = build_issue_body(system_prompt, config, desc, mechanical=False)
        body += f"\n\n{common.make_marker(AGENT_NAME, key)}"
        try:
            common.create_issue(config, title, body, ISSUE_LABELS, AGENT_NAME)
        except common.AgentError as exc:
            common.log(f"No se pudo crear el issue para {key}: {exc}. Se continúa con el siguiente hallazgo.")

    if ci_run:
        key = f"ci-failed-{ci_run.get('head_sha') or ci_run.get('id')}"
        title = f"bug: la última ejecución de CI en {CI_BRANCH} no fue exitosa"
        desc = (
            f"La última ejecución completada de `{CI_WORKFLOW_FILE}` sobre `{CI_BRANCH}` terminó con "
            f"conclusion=`{ci_run.get('conclusion')}`.\n\n"
            f"Ejecución: {ci_run.get('html_url')}\n\n"
            "Este chequeo reutiliza el resultado ya calculado por CI (no re-ejecuta la suite de "
            "tests): cubre la suite CPU (Catch2) que `ci.yml` ejecuta vía `make test`. Los tests "
            "CUDA solo se compilan en CI (no hay GPU en el runner de Actions), por lo que su "
            "ejecución no queda cubierta por este chequeo."
        )
        human_reason = "diagnosticar la causa real de un test roto requiere leer el log de CI y el código, no solo su resultado"
        if config.dry_run:
            body = build_issue_body(system_prompt, config, desc, mechanical=False, human_reason=human_reason)
            common.log(f"[dry-run] Hallazgo: {title}\n{body}")
        elif not common.issue_marker_exists(config, ISSUE_LABELS[1], AGENT_NAME, key):
            body = build_issue_body(system_prompt, config, desc, mechanical=False, human_reason=human_reason)
            body += f"\n\n{common.make_marker(AGENT_NAME, key)}"
            try:
                common.create_issue(config, title, body, ISSUE_LABELS, AGENT_NAME)
            except common.AgentError as exc:
                common.log(f"No se pudo crear el issue para {key}: {exc}. Se continúa con el siguiente hallazgo.")
        else:
            common.log(f"Ya existe un issue para {key}; se omite.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
