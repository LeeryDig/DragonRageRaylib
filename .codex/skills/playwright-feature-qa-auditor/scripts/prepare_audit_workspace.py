#!/usr/bin/env python3

import argparse
import json
import re
from datetime import datetime
from pathlib import Path


DEFAULT_AUDIT_DIR = "qa-audits"


def slugify(value: str) -> str:
    slug = re.sub(r"[^a-zA-Z0-9]+", "-", value.strip().lower()).strip("-")
    return slug or "feature"


def normalize_pattern(audit_dir: str) -> str:
    return audit_dir.replace("\\", "/").strip("/") + "/"


def ensure_git_exclude(project_root: Path, pattern: str) -> bool:
    exclude_path = project_root / ".git" / "info" / "exclude"
    if not exclude_path.exists():
        return False

    existing = exclude_path.read_text(encoding="utf-8")
    lines = [line.strip() for line in existing.splitlines()]
    if pattern in lines:
        return False

    prefix = "" if not existing or existing.endswith("\n") else "\n"
    exclude_path.write_text(f"{existing}{prefix}{pattern}\n", encoding="utf-8")
    return True


def unique_run_dir(base_dir: Path, run_name: str) -> Path:
    candidate = base_dir / run_name
    counter = 1
    while candidate.exists():
        candidate = base_dir / f"{run_name}-{counter}"
        counter += 1
    return candidate


def write_text(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def build_report_template(feature: str, created_at: str) -> str:
    return f"""# Auditoria de QA

## Escopo

- Feature: {feature}
- Objetivo:
- Ambiente ou URL:
- Data: {created_at}
- Responsavel:

## Contexto

- Branch ou commit:
- Credenciais ou dados de teste:
- Premissas:
- Fora de escopo:

## Cenarios executados

| ID | Cenario | Resultado | Evidencia |
| --- | --- | --- | --- |
| C-01 | | | |

## Resumo executivo

- Status geral:
- Bugs encontrados:
- Bloqueios:
- Riscos residuais:

## Cobertura e gaps

- Coberto:
- Nao coberto:
- Proximos passos:
"""


def build_findings_template(feature: str) -> str:
    return f"""# Findings

## Contexto

- Feature: {feature}
- Registrar apenas falhas reais, bloqueios e comportamentos fora do esperado.

## F-001 - Titulo curto

- Severidade:
- Estado:
- Pre-condicao:
- Passos para reproduzir:
  1. 
  2. 
  3. 
- Resultado esperado:
- Resultado observado:
- Evidencia:
- Impacto:
- Observacao:
"""


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare a local QA audit workspace and update .git/info/exclude."
    )
    parser.add_argument("--project-root", required=True, help="Target project root")
    parser.add_argument("--feature", required=True, help="Feature or flow under test")
    parser.add_argument(
        "--audit-dir",
        default=DEFAULT_AUDIT_DIR,
        help="Relative audit directory inside the project",
    )
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    audit_dir = args.audit_dir.strip().strip("/\\") or DEFAULT_AUDIT_DIR
    base_dir = project_root / audit_dir
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    run_name = f"{timestamp}-{slugify(args.feature)}"
    run_dir = unique_run_dir(base_dir, run_name)

    artifacts_dir = run_dir / "artifacts"
    screenshots_dir = artifacts_dir / "screenshots"
    traces_dir = artifacts_dir / "traces"
    videos_dir = artifacts_dir / "videos"

    for path in (screenshots_dir, traces_dir, videos_dir):
        path.mkdir(parents=True, exist_ok=True)

    created_at = datetime.now().isoformat(timespec="seconds")
    report_path = run_dir / "report.md"
    findings_path = run_dir / "findings.md"
    metadata_path = run_dir / "metadata.json"

    write_text(report_path, build_report_template(args.feature, created_at))
    write_text(findings_path, build_findings_template(args.feature))

    exclude_pattern = normalize_pattern(audit_dir)
    exclude_updated = ensure_git_exclude(project_root, exclude_pattern)

    metadata = {
        "feature": args.feature,
        "created_at": created_at,
        "project_root": str(project_root),
        "audit_dir": str(base_dir),
        "run_dir": str(run_dir),
        "report_path": str(report_path),
        "findings_path": str(findings_path),
        "artifacts": {
            "root": str(artifacts_dir),
            "screenshots": str(screenshots_dir),
            "traces": str(traces_dir),
            "videos": str(videos_dir),
        },
        "git_exclude_pattern": exclude_pattern,
        "git_exclude_updated": exclude_updated,
    }
    write_text(metadata_path, json.dumps(metadata, indent=2))

    print(json.dumps(metadata, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
