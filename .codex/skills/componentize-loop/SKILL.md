---
name: componentize-loop
description: Componentizar arquivos grandes (principalmente UI/React e codigo com muitas responsabilidades) em partes menores e reutilizaveis, reduzindo complexidade e duplicacao sem mudar comportamento. Use quando o usuario pedir componentizar, quebrar componente grande, refatorar tela, separar em componentes, reaproveitar partes, split component, organizar UI ou modularizar componente.
---

# Componentize Loop

## Overview

Refactor large files in small iterations, extracting reusable components, hooks, and utilities while preserving behavior and public APIs.
Run an execution loop until componentization is complete and repository validations are green.

## Mandatory Rules

1. Keep exclusive focus on componentization and reuse.
2. Extract reusable UI subcomponents and reusable logic functions.
3. Extract repeated blocks into a single shared component or utility.
4. Do not change behavior or public API unless user explicitly asks.
5. Work in small diffs per iteration, but continue iterating until complete.
6. Preserve import/export compatibility:
- Keep original file as container when needed.
- Keep original main export to avoid breaking external imports.
7. Prefer composition over inheritance.
8. Keep props simple and typed when using TypeScript.
9. Use only existing repository commands for validation. Never invent commands.
10. Preserve and adjust existing tests; add minimal tests only when regression risk is real.

## Componentization Patterns (Required)

1. Extract by responsibility:
- `Header`
- `Filters`
- `Table`
- `Modal`
- `FormSection`
- `Footer`
2. Extract hooks (`useXyz`) for complex state/effects logic.
3. Extract helpers/utilities for pure functions and repeated formatting/validation.
4. Avoid excessive prop drilling.
5. Introduce local domain context only when that is already a repository pattern.

## Target Structure (Adapt To Repo)

For a large page/component, prefer:

`src/features/<feature>/pages/<Page>.tsx`

Create:

- `src/features/<feature>/components/<Page>/index.ts`
- `src/features/<feature>/components/<Page>/<Page>Container.tsx` (optional)
- `src/features/<feature>/components/<Page>/<Page>Header.tsx`
- `src/features/<feature>/components/<Page>/<Page>Filters.tsx`
- `src/features/<feature>/components/<Page>/<Page>Table.tsx`
- `src/features/<feature>/components/<Page>/<Page>Modal.tsx`
- `src/features/<feature>/components/<Page>/hooks/use<Page>State.ts`
- `src/features/<feature>/components/<Page>/utils/format.ts`
- `src/features/<feature>/components/<Page>/utils/validators.ts`

Keep the original public export path working.

## Loop Process (Always Repeat Until Done)

1. Map sections of the large file (UI and logic).
2. Select one section to extract (largest or most repeated first).
3. Extract to a new component, hook, or utility with clear interfaces.
4. Replace original section and verify behavior is unchanged.
5. Run repository validations that exist (`lint`, `test`, `build`, `typecheck`, smoke if available).
6. Fix immediately if anything fails.
7. Repeat loop.

## Completion Criteria (Do Not Stop Earlier)

1. Original file is a clean container or significantly smaller and readable.
2. Extracted components are reusable and not unnecessarily coupled.
3. Duplicated blocks/functions are removed.
4. Existing project validations are green.

## Required Chat Output

Always return:

- `Resumo do que foi componentizado`
- `Estrutura final de arquivos/pastas criada`
- `Lista de arquivos alterados/criados`
- `Comandos reais para validar (lint/test/build/test)`
- `Riscos/impactos e rollback`

If any command fails during the loop, include what was fixed before rerunning.
