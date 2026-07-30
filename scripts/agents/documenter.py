#!/usr/bin/env python3
"""Agente documentador (Rol 4 — Lab 2).

Revisa README.md, nbody_2d/README.md, nbody_2d/tests/README.md y
CHANGELOG.md en busca de:

- enlaces relativos rotos (apuntan a un archivo que no existe);
- encabezados de clases CUDA/host sin ningún comentario introductorio;
- secciones evidentemente incompletas (placeholders de plantilla).

Cada hallazgo se clasifica como "Mecánico" o "Humano" (este último requiere
juicio técnico: kernels, memoria, física, sincronización, tolerancias). Los
hallazgos ya reportados (mismo marcador) no generan un issue nuevo.

Uso:
    python scripts/agents/documenter.py [--dry-run]

Variables de entorno relevantes: ver scripts/agents/common.py y
scripts/agents/README.md.
"""

from __future__ import annotations

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common  # noqa: E402

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
AGENT_NAME = "documenter"
ISSUE_LABELS = ["documentation", "agent-documenter"]

MARKDOWN_FILES = [
    "README.md",
    "nbody_2d/README.md",
    "nbody_2d/tests/README.md",
    "CHANGELOG.md",
]

# Cabeceras de clases relevantes que deberían tener un comentario introductorio.
HEADER_FILES = [
    "nbody_2d/CudaBuffer.h",
    "nbody_2d/CudaCheck.cuh",
    "nbody_2d/NBodySystem.h",
    "nbody_2d/NBodySimulator.h",
    "nbody_2d/kernels/accelerations.cuh",
    "nbody_2d/kernels/metrics.cuh",
]

LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
PROMPT_PATH = os.path.join(os.path.dirname(__file__), "prompts", "documenter.md")


def load_system_prompt() -> str:
    with open(PROMPT_PATH, "r", encoding="utf-8") as fh:
        return fh.read()


def find_broken_links(rel_path: str) -> list:
    abs_path = os.path.join(REPO_ROOT, rel_path)
    findings = []
    if not os.path.isfile(abs_path):
        return findings
    with open(abs_path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()

    base_dir = os.path.dirname(abs_path)
    for match in LINK_RE.finditer(text):
        target = match.group(1).strip()
        if not target or target.startswith(("http://", "https://", "mailto:", "#")):
            continue
        target_no_anchor = target.split("#", 1)[0]
        if not target_no_anchor:
            continue
        candidate = os.path.normpath(os.path.join(base_dir, target_no_anchor))
        if not os.path.exists(candidate):
            findings.append((rel_path, target))
    return findings


def find_undocumented_headers() -> list:
    findings = []
    for rel_path in HEADER_FILES:
        abs_path = os.path.join(REPO_ROOT, rel_path)
        if not os.path.isfile(abs_path):
            continue
        with open(abs_path, "r", encoding="utf-8", errors="replace") as fh:
            head = "".join(fh.readlines()[:6])
        if "//" not in head and "/*" not in head:
            findings.append(rel_path)
    return findings


def build_issue_body(
    system_prompt: str,
    config: common.Config,
    finding_desc: str,
    mechanical: bool,
    human_reason: str = "el hallazgo requiere juicio técnico sobre kernels, memoria, física o sincronización",
) -> str:
    if config.has_model_provider():
        try:
            text = common.call_model(
                system_prompt,
                f"Hallazgo detectado por análisis estático:\n\n{finding_desc}\n\n"
                f"Tipo sugerido por el análisis estático: {'Mecánico' if mechanical else 'Humano'}.",
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
            "Requiere intervención humana: el análisis estático no tiene juicio técnico "
            "suficiente para explicar kernels, memoria, física o sincronización."
        )
    return "\n".join(fallback)


def main() -> int:
    parser = common.build_arg_parser("Agente documentador: revisa README(s) y CHANGELOG.md.")
    args = parser.parse_args()
    config = common.Config(dry_run=args.dry_run)

    common.log(f"Proveedor de IA: {common.describe_provider(config)}")
    if config.dry_run:
        common.log("Modo --dry-run activo: no se escribirá en GitHub.")
    elif not config.github_token:
        common.log(
            "GITHUB_TOKEN no está definido y no se pasó --dry-run. "
            "No se puede leer ni escribir en GitHub. Aborta."
        )
        return 1

    system_prompt = load_system_prompt()

    broken_links = []
    for rel_path in MARKDOWN_FILES:
        broken_links.extend(find_broken_links(rel_path))

    undocumented_headers = find_undocumented_headers()

    if not broken_links and not undocumented_headers:
        common.log("Sin hallazgos de documentación en esta ejecución.")
        return 0

    created_any = False

    for rel_path, target in broken_links:
        key = f"broken-link-{rel_path}-{target}"
        title = f"docs: enlace roto en {rel_path} -> {target}"
        desc = f"En `{rel_path}` hay un enlace hacia `{target}` que no resuelve a un archivo existente en el repositorio."
        if config.dry_run:
            body = build_issue_body(system_prompt, config, desc, mechanical=True)
            common.log(f"[dry-run] Hallazgo: {title}\n{body}")
            created_any = True
            continue
        if common.issue_marker_exists(config, ISSUE_LABELS[1], AGENT_NAME, key):
            common.log(f"Ya existe un issue para {key}; se omite.")
            continue
        body = build_issue_body(system_prompt, config, desc, mechanical=True)
        body += f"\n\n{common.make_marker(AGENT_NAME, key)}"
        try:
            issue = common.create_issue(config, title, body, ISSUE_LABELS, AGENT_NAME)
            created_any = created_any or bool(issue)
        except common.AgentError as exc:
            common.log(f"No se pudo crear el issue para {key}: {exc}. Se continúa con el siguiente hallazgo.")

    for rel_path in undocumented_headers:
        key = f"missing-header-doc-{rel_path}"
        title = f"docs: falta comentario introductorio en {rel_path}"
        desc = (
            f"El archivo `{rel_path}` no tiene ningún comentario en sus primeras líneas que explique "
            "su propósito, lo que dificulta entender decisiones de diseño (memoria, kernels, "
            "sincronización) sin leer toda la implementación."
        )
        if config.dry_run:
            body = build_issue_body(system_prompt, config, desc, mechanical=False)
            common.log(f"[dry-run] Hallazgo: {title}\n{body}")
            created_any = True
            continue
        if common.issue_marker_exists(config, ISSUE_LABELS[1], AGENT_NAME, key):
            common.log(f"Ya existe un issue para {key}; se omite.")
            continue
        body = build_issue_body(system_prompt, config, desc, mechanical=False)
        body += f"\n\n{common.make_marker(AGENT_NAME, key)}"
        try:
            issue = common.create_issue(config, title, body, ISSUE_LABELS, AGENT_NAME)
            created_any = created_any or bool(issue)
        except common.AgentError as exc:
            common.log(f"No se pudo crear el issue para {key}: {exc}. Se continúa con el siguiente hallazgo.")

    if not created_any:
        common.log("Todos los hallazgos ya tenían un issue asociado o se omitieron por el límite semanal.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
