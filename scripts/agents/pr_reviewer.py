#!/usr/bin/env python3
"""Agente revisor de pull requests (Rol 4 — Lab 2).

Pensado para dispararse mediante `workflow_run` cuando termina el workflow
de CI (`ci.yml`). Encuentra la PR asociada al commit evaluado, lee su diff
y el resultado del CI, y publica **un** comentario por commit (nunca
duplicado) clasificando el cambio como mecánico y revisable, o como que
requiere revisión humana. Nunca aprueba ni fusiona la PR.

Modo de uso normal (dentro del workflow, disparado por `workflow_run`):
    python scripts/agents/pr_reviewer.py [--dry-run]

Esto lee el evento desde `GITHUB_EVENT_PATH` (payload de `workflow_run`).

Modo manual/pruebas:
    python scripts/agents/pr_reviewer.py --pr-number 12 --sha <sha> \\
        --conclusion success --run-url https://... [--dry-run]
"""

from __future__ import annotations

import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common  # noqa: E402

AGENT_NAME = "pr-reviewer"
PROMPT_PATH = os.path.join(os.path.dirname(__file__), "prompts", "pr_reviewer.md")

PHYSICS_SENSITIVE_RE = re.compile(
    r"nbody_2d/(kernels/.*\.(cu|cuh)|Integrator\.(cpp|h)|NBodySimulator\.(cpp|h)|"
    r"NBodySystem\.(cpp|h)|MetricsCalculator\.(cpp|h)|Particle\.(cpp|h)|"
    r"CudaBuffer\.h|CudaCheck\.cuh)$"
)
MECHANICAL_ALLOWED_RE = re.compile(r"\.(md|yml|yaml|txt)$|(^|/)\.gitignore$")
# Valida la SINTAXIS de vinculación a un issue (palabras clave de cierre de
# GitHub: closes/close/closed, fixes/fix/fixed, resolves/resolve/resolved,
# más refs/references, que no auto-cierran pero sí vinculan). No consulta la
# API para confirmar que el issue referenciado exista de verdad.
ISSUE_REF_RE = re.compile(
    r"\b(close[sd]?|fix(?:e[sd])?|resolve[sd]?|refs?|references?)\s*:?\s*#\d+",
    re.IGNORECASE,
)
DIFF_FILE_RE = re.compile(r"^diff --git a/(.+?) b/(.+?)$", re.MULTILINE)


def load_system_prompt() -> str:
    with open(PROMPT_PATH, "r", encoding="utf-8") as fh:
        return fh.read()


def parse_changed_files(diff_text: str) -> list:
    return [m.group(2) for m in DIFF_FILE_RE.finditer(diff_text)]


def classify_change(ci_success: bool, changed_files: list, pr_body: str) -> tuple:
    if not ci_success:
        return False, "El CI no terminó en éxito."
    if not changed_files:
        return False, "No se pudieron determinar los archivos modificados."
    if any(PHYSICS_SENSITIVE_RE.search(f) for f in changed_files):
        return False, "El diff toca archivos de física, kernels o memoria CUDA."
    if not all(MECHANICAL_ALLOWED_RE.search(f) for f in changed_files):
        return False, "El diff incluye archivos fuera de documentación/formato/configuración evidente."
    if not ISSUE_REF_RE.search(pr_body or ""):
        return False, (
            "La PR no vincula un issue con una palabra clave de cierre reconocida "
            "(p. ej. 'Closes #7', 'Fixes #7', 'Resolves #7', 'Refs #7'); no se valida "
            "si el issue referenciado existe realmente, solo la sintaxis de vinculación."
        )
    return True, "CI en verde, solo documentación/formato/configuración, y con issue asociado."


def read_workflow_run_event() -> dict:
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if not event_path or not os.path.isfile(event_path):
        raise common.AgentError(
            "GITHUB_EVENT_PATH no está disponible; ejecuta este script dentro del workflow "
            "disparado por workflow_run, o usa --pr-number/--sha/--conclusion/--run-url "
            "para pruebas manuales."
        )
    with open(event_path, "r", encoding="utf-8") as fh:
        payload = json.load(fh)
    run = payload.get("workflow_run")
    if not run:
        raise common.AgentError("El evento recibido no es un evento 'workflow_run' con datos utilizables.")
    return {
        "sha": run.get("head_sha"),
        "conclusion": run.get("conclusion"),
        "run_url": run.get("html_url"),
        "run_id": run.get("id"),
        "name": run.get("name"),
    }


MERGE_REMINDER = (
    "Recordatorio: el merge de esta PR lo realiza una persona humana; este comentario "
    "no es una aprobación ni una autorización de merge."
)


def build_comment(system_prompt: str, config: common.Config, context: dict) -> str:
    ci_line = "CI: ÉXITO" if context["ci_success"] else f"CI: FALLO/NO EXITOSO (conclusion={context['conclusion']})"
    files_list = "\n".join(f"- `{f}`" for f in context["changed_files"][:30]) or "(sin archivos detectados)"

    text = None
    if config.has_model_provider():
        try:
            user_prompt = (
                f"Resultado de CI: {context['conclusion']}\n"
                f"URL de la ejecución de CI: {context['run_url']}\n"
                f"Archivos modificados:\n{files_list}\n\n"
                f"Clasificación calculada por análisis estático: "
                f"{'Mecánico y revisable' if context['mechanical'] else 'Requiere revisión humana'} "
                f"({context['reason']}).\n\n"
                f"Fragmento de diff (recortado):\n{context['diff_excerpt']}"
            )
            text = common.call_model(system_prompt, user_prompt, config)
        except common.AgentError as exc:
            common.log(f"Fallo al llamar al proveedor de IA, se usa comentario estático: {exc}")

    if text is None:
        classification = "Cambio mecánico y revisable" if context["mechanical"] else "Requiere revisión humana"
        text = (
            "**ANÁLISIS ESTÁTICO (sin modelo de IA disponible)**\n\n"
            f"- {ci_line}\n"
            f"- Ejecución de CI: {context['run_url']}\n"
            f"- Clasificación: **{classification}** ({context['reason']})\n\n"
            f"Archivos relevantes:\n{files_list}\n\n"
            "Riesgos detectados: ninguno adicional al motivo de clasificación indicado arriba.\n\n"
            "Recomendación: revisar el diff completo antes de fusionar."
        )

    # Garantías forzadas por código: nunca depender solo de que el modelo siguió el
    # prompt. Si el CI falló, o el cambio no es mecánico (kernels/física/API/CI-fail/
    # sin issue asociado), la frase de intervención humana debe estar presente.
    if not context["mechanical"]:
        text = common.ensure_human_notice(text, context["reason"])

    # El recordatorio de que el merge es humano debe aparecer siempre, en todo comentario.
    if MERGE_REMINDER.lower() not in text.lower():
        text = f"{text}\n\n{MERGE_REMINDER}"

    return text


def main() -> int:
    parser = common.build_arg_parser("Agente revisor de PR: comenta tras finalizar el CI de una PR.")
    parser.add_argument("--pr-number", type=int, help="Número de PR (modo manual)")
    parser.add_argument("--sha", help="SHA del commit evaluado por CI (modo manual)")
    parser.add_argument("--conclusion", help="Conclusion del workflow de CI (modo manual)")
    parser.add_argument("--run-url", help="URL de la ejecución de CI (modo manual)")
    args = parser.parse_args()
    config = common.Config(dry_run=args.dry_run)

    common.log(f"Proveedor de IA: {common.describe_provider(config)}")
    if config.dry_run:
        common.log("Modo --dry-run activo: no se escribirá en GitHub.")
    elif not config.github_token:
        common.log("GITHUB_TOKEN no está definido y no se pasó --dry-run. Aborta.")
        return 1

    if args.sha and args.conclusion:
        run_info = {
            "sha": args.sha,
            "conclusion": args.conclusion,
            "run_url": args.run_url or "(no proporcionada)",
            "run_id": None,
            "name": "manual",
        }
    else:
        try:
            run_info = read_workflow_run_event()
        except common.AgentError as exc:
            common.log(str(exc))
            return 1 if not config.dry_run else 0

    sha = run_info["sha"]
    if not sha:
        common.log("No se pudo determinar el SHA del commit evaluado. Aborta.")
        return 1

    ci_success = run_info["conclusion"] == "success"

    if args.pr_number:
        pr_numbers = [args.pr_number]
    else:
        try:
            prs = common.get_pulls_for_commit(config, sha)
        except common.AgentError as exc:
            common.log(f"No se pudo buscar la PR asociada al commit {sha}: {exc}")
            return 1 if not config.dry_run else 0
        pr_numbers = [pr["number"] for pr in prs if pr.get("state") == "open"]

    if not pr_numbers:
        common.log(f"No hay ninguna PR abierta asociada al commit {sha}; nada que comentar.")
        return 0

    system_prompt = load_system_prompt()

    for number in pr_numbers:
        try:
            pr = common.get_pull(config, number)
            diff_text = common.get_pull_diff(config, number)
        except common.AgentError as exc:
            common.log(f"No se pudo leer la PR #{number} o su diff: {exc}")
            continue

        changed_files = parse_changed_files(diff_text)
        mechanical, reason = classify_change(ci_success, changed_files, pr.get("body", ""))
        context = {
            "ci_success": ci_success,
            "conclusion": run_info["conclusion"],
            "run_url": run_info["run_url"],
            "changed_files": changed_files,
            "mechanical": mechanical,
            "reason": reason,
            "diff_excerpt": diff_text[:4000],
        }
        comment = build_comment(system_prompt, config, context)
        common.create_or_update_pr_comment(config, number, comment, AGENT_NAME, key=sha)

    return 0


if __name__ == "__main__":
    sys.exit(main())
