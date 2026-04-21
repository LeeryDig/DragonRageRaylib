---
name: implement-audit
description: Fazer double check de implementacoes recentes, validar funcionamento ponta a ponta, corrigir gaps ate ficar verde e registrar auditoria datada local em audit_logs/, sem versionar esses relatorios. Use quando o usuario pedir double check, auditar, validar tudo, revisar implementacao, garantir sem gaps, verificar se continua funcionando, rodar testes e corrigir, ou sanity check.
---

# Implement Audit

## Overview

Audit recent implementation changes, execute repository-native validations, fix issues until all required checks pass, and persist an auditable report under `audit_logs/`.
Use only commands and conventions that already exist in the target repository.

## Mandatory Rules

1. Define "recently implemented" using git first:
- `git diff` (working tree + staged)
- recent commits and touched files
2. If git is unavailable, infer scope from files modified in the current session and visible recent changes in repo files.
3. Run applicable validations only when they exist in the repo:
- lint/format
- tests
- typecheck/build
- smoke run (when there is an existing ready command)
4. Never invent commands. Use only commands/scripts already defined in project files (for example: `package.json`, `Makefile`, `pyproject.toml`, task scripts, existing CI/local scripts).
5. If any validation fails, fix and rerun until green.
6. If relevant test coverage is missing for bugfixes or critical rules, create or adjust tests.
7. Always generate a dated local audit report in `audit_logs/`.
8. Keep `audit_logs/` in the local Git exclude list (`.git/info/exclude`) and do not require repository-wide `.gitignore` changes for this folder.
9. Do not version or commit files from `audit_logs/`.

## Audit Logs Requirements

1. Create `audit_logs/` at repository root when missing.
2. Create one file per run with timestamp format:
- `audit_logs/YYYY-MM-DD_HH-mm_implement-audit.md`
3. Write the report with all sections below:
- Escopo auditado (arquivos e funcionalidades)
- Checklist do que foi validado
- Comandos executados
- Resultados (pass/fail) com trechos curtos do output quando relevante
- Gaps encontrados e como foram corrigidos
- Arquivos alterados (lista)
- Riscos restantes (se houver) e recomendacoes

## Local Git Exclude Rule

Keep `audit_logs/` in the local Git exclude file:

- `.git/info/exclude`
- `audit_logs/`

Do not instruct changes to repository `.gitignore` for audit artifacts.
Do not create `audit_logs/.keep` only to force tracking.
Treat audit reports as local artifacts only.

## Workflow (Always Follow)

1. Detect stack and locate existing validation commands.
2. Determine "recent" scope from git diff/commits or fallback file evidence.
3. Execute validations in applicable groups:
- lint/format
- tests
- typecheck/build
- smoke
4. If any failure occurs:
- fix implementation
- add/adjust tests when coverage is missing
- rerun validations
- repeat until green
5. Generate/update the audit report file in `audit_logs/` with final results.
6. Return final chat response with required sections and actual commands used.

## Output Format (Mandatory)

Always return:

- `Checklist do que foi verificado`
- `Comandos usados (reais do repo)`
- `Resultado final`
- `Arquivo de auditoria gerado em audit_logs/`

Also include:

- resumo do escopo auditado
- status final verde + lista de comandos que passaram
- lista de arquivos alterados

When there are remaining risks, state them explicitly in both report and chat.
