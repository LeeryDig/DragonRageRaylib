---
name: test-coverage-audit
description: Auditar suites de testes e cobertura do repositorio, garantir comando oficial de coverage com mudanca minima quando faltar, executar validacoes reais e gerar relatorio versionado em test_reports/. Use quando o usuario pedir testes unitarios, cobertura, coverage, relatorio de testes, qual cobertura, garantir testes, ou auditar testes.
---

# Test Coverage Audit

## Overview

Detect test tooling from the repository, run real test and coverage commands, and report current health by suite type.
When coverage or essential tests are missing, implement the minimum stack-native setup and document exactly what changed.

## Mandatory Rules

1. Auto-detect stack and existing commands/configuration from repository files.
2. Search command sources before running anything:
- `package.json` scripts (`test`, `test:unit`, `coverage`, etc.)
- `Makefile`
- `pyproject.toml`, `setup.cfg`, `tox.ini`, Poetry config
- test configs (`jest`, `vitest`, `pytest`, `coverage.py`, `nyc`, etc.)
3. Never invent ad-hoc commands for execution. Use only commands that exist in repo.
4. If no official coverage command exists, implement minimal stack-native changes to create one.
5. Always distinguish and report:
- unit tests (priority)
- integration tests (when present)
- e2e tests (when present)
6. If obvious gaps are found (critical functions/features without tests), add minimum unit tests for main flow and edge cases.
7. Always generate a versioned report in `test_reports/`.
8. Keep changes minimal and aligned with detected stack.

## Coverage and Test Expectations

Always provide objective output:

- which suites exist
- where tests are located
- how to run each suite
- coverage summary (overall and by module/directory when available)
- top modules/files with low coverage (top 10 when possible)

If coverage tooling already exists, run it and collect results.
If it does not exist, enable it with minimal standard setup for detected stack and create an official repo command.

## Test Reports Requirements

1. Create `test_reports/` at repository root when missing.
2. Ensure it is committable with minimal `.gitignore` changes:
- `!test_reports/`
- `!test_reports/**`
3. Create one report per execution:
- `test_reports/YYYY-MM-DD_HH-mm_test-coverage-audit.md`
4. Include in report:
- Stack detectado e ferramentas de teste encontradas
- Suites de testes existentes (unit/integration/e2e)
- Comandos reais usados para rodar testes e coverage
- Resultado final (pass/fail)
- Cobertura total e por diretorio/modulo (quando disponivel)
- Lista de arquivos/modulos com baixa cobertura (top 10)
- Sugestoes objetivas de onde adicionar testes
- Arquivos alterados/criados (quando houver mudancas)

## Workflow (Always Follow)

1. Detect stack and discover existing scripts/config files for tests and coverage.
2. Verify presence of unit tests (priority), then integration/e2e where applicable.
3. Verify whether coverage command exists and how to generate report.
4. Run tests and coverage using real repository commands.
5. If no official coverage path exists, implement minimal setup and add official command.
6. If critical gaps exist, add minimum unit tests for main and edge flows.
7. Rerun affected commands to confirm final state.
8. Generate `test_reports` report and return final summary in chat.

## Chat Output Format (Mandatory)

Always return:

- `Checklist do que foi verificado`
- `Comandos usados (reais do repo)`
- `Resultado final`
- `Arquivo de auditoria gerado em test_reports/`

Also include:

- status dos testes
- cobertura encontrada
- comandos para reproduzir
- lista de arquivos alterados/criados

When coverage by module is unavailable from tool output, state that explicitly and provide best available breakdown.
