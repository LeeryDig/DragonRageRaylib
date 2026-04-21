---
name: docs-guardian
description: Organizar, validar e padronizar a documentacao do projeto em docs/ no root, garantindo existencia de docs/indice.md, navegacao por contexto, estrutura numerada por dominio e coerencia com o codigo real (sem inventar comandos, env vars, endpoints ou fluxos). Use quando o usuario pedir por documentacao/docs, revisar docs, validar docs, organizar docs, padronizar docs, indice, coerencia, atualizar readme ou criar docs.
---

# Docs Guardian

## Overview

Organize and validate project documentation under `docs/` with a strict, domain-oriented structure and a single navigation source in `docs/indice.md`.
Keep all content aligned with what actually exists in the repository.

## Mandatory Rules

1. Always verify whether `docs/` exists at repository root.
2. Always guarantee `docs/indice.md` exists.
3. Build `docs/indice.md` with one section per existing folder in `docs/`, listing links to all `.md` files in that folder.
4. Use domain-oriented folders with numeric prefixes (`NN_Name`) to improve ordering.
5. Create only folders/files that are applicable to the project and requested by the user.
6. If `docs/` does not exist, create a minimal structure first, then expand only when needed.
7. Never invent commands, scripts, env vars, endpoints, contracts, or flows.
8. If documentation conflicts exist, pick one version grounded in repository truth and standardize.

## Domain Structure

Use only applicable folders below:

- `docs/01_Overview/` for product/service overview
- `docs/02_Setup/` for install, prerequisites, local run, env
- `docs/03_Architecture/` for architecture, components, flows, data
- `docs/04_API/` for endpoints/contracts (only when API exists)
- `docs/05_Data/` for data models/schemas/migrations (only when applicable)
- `docs/06_Operations/` for deploy/observability/runbooks (only when applicable)
- `docs/07_Security/` for secrets/permissions/compliance (only when applicable)
- `docs/08_Guides/` for how-tos, troubleshooting, contribution

Inside each folder, keep file names numerically prefixed: `01_`, `02_`, `03_`... with coherent names.
Do not force a fixed number of files.

## Minimal Bootstrap

If missing, create:

- `docs/indice.md`
- `docs/01_Overview/01_visao_geral.md` only if it makes sense for the project
- `docs/02_Setup/01_rodar_local.md` only if the project runs locally

`docs/README.md` is optional and may only point to `docs/indice.md`.

## Workflow

1. Check existence of `docs/` and `docs/indice.md`.
2. Map all current documentation under `docs/`:
- Existing folders
- Existing `.md` files
- Naming/numbering consistency
3. Create or adjust structure only where needed.
4. Rebuild or update `docs/indice.md` as the main navigation file:
- One section per existing folder
- All `.md` files listed with valid relative links
5. Validate links from `docs/indice.md` and remove broken references.
6. Cross-check docs against repository reality before writing:
- Commands from actual scripts/Makefile/package scripts/README or equivalent
- Env vars from existing config/sample files
- Endpoints/contracts from real code/specs
7. Standardize contradictions using repository truth.
8. Produce the mandatory report format.

## Validation Criteria

Classify findings as:

- Critical failures: missing `docs/`, missing `docs/indice.md`, broken links
- Medium failures: incomplete setup docs, missing env vars, contradictions
- Improvements: clarity, organization, real examples grounded in code

## Required Output Format

Always answer with these sections, in this order:

1. `Checklist do que foi verificado`
2. `Lista de arquivos criados/alterados`
3. `Resumo do que mudou`
4. `Passos de validacao local (somente comandos existentes no repo)`

In the last section, include only commands that are present and valid for the target repository.
