---
name: execution-orchestration-governor
description: Govern PRD execution across multiple modules with explicit state tracking, audit gates, reopen rules, and delegation discipline. Use when Codex needs to coordinate work between `@produto`, `@infraestrutura`, `@frontend`, `@backend`, `@mongodb`, `@qualidade`, and `@auditoria`, especially in monorepos with more than one module, PRD, or infrastructure PRD.
---

# Execution Orchestration Governor

Use this skill to make orchestration traceable instead of conversational.

## Core workflow

1. Read the central tracker first: `docs/RASTREABILIDADE_PRDS.md`.
   - then read `docs/PADRAO_RASTREABILIDADE_EXECUCAO.md`
2. Read the target module history next:
   - `backend-authenticator-qr-code/docs/HISTORICO_ANDAMENTO.md`
   - `backend-tribunal-status/docs/HISTORICO_ANDAMENTO.md`
   - `frontend-backoffice/docs/HISTORICO_ANDAMENTO.md`
3. Decide whether the module is:
   - already closed and should not be reopened now
   - open and needs the next specialist agent
   - blocked and needs user input
4. Determine the next specialist through the mandatory routing matrix in `references/agent-routing-matrix.md`.
5. Delegate only the smallest scoped next step.
6. Call `@qualidade` after each specialist correction round.
7. Call `@auditoria` after the QA pass.
8. If `@qualidade` or `@auditoria` rejects, turn the findings into explicit gaps and dispatch them back to the responsible specialist.
9. Update the module history, central tracker, and dispatch log before stopping.
10. If the gap is clear, reproducible, in-scope, and assigned to an obvious specialist, do not pause to ask the user whether to continue. Continue the loop automatically.

## Dispatch-first rule

The orchestrator is a coordinator first.

Default behavior:
- dispatch to the responsible specialist agent
- keep local work limited to:
  - tracker updates
  - history updates
  - dispatch log updates
  - tiny cross-cutting glue edits that do not belong clearly to a specialist

Do not implement substantial backend, frontend, database, product, QA, or infrastructure work locally when a specialist agent exists for that scope.

The orchestrator must behave as a multi-agent sequencer, not as a single-step dispatcher.

Use this mapping by default:
- `@produto` for PRD creation, PRD updates, acceptance criteria, and roadmap decisions
- `@infraestrutura` for Docker, Terraform, GitHub Actions, Azure, env contracts, and deployment readiness
- `@backend` for backend code, API contracts, service logic, persistence, tests, and runtime fixes
- `@frontend` for UI, browser-facing runtime, styling, UX flow, forms, and frontend tests
- `@mongodb` for MongoDB ownership, collection strategy, indexes, DBML, and persistence boundaries
- `@qualidade` for mandatory runtime QA evidence and browser checks before audit acceptance
- `@auditoria` for approval or missing items

This mapping is not optional guidance. The orchestrator must choose from it explicitly before doing substantial local work.

Only skip dispatch when:
- there is no suitable specialist agent
- the change is purely orchestration bookkeeping
- the user explicitly asked not to delegate
- dispatch failed operationally, such as thread limit, agent timeout, or no usable response after a reasonable retry

Whenever dispatch happens, update `logs/LOG_DESPACHOS_ORQUESTRADOR.md`.

If dispatch fails operationally:
- log the failed dispatch attempt
- retry once with another available agent when reasonable
- if the retry also fails, execute locally as fallback
- still route the result through `@auditoria`

## Mandatory routing matrix

Read and apply:

- `references/agent-routing-matrix.md`

Rules:
- choose exactly one next specialist owner before each round
- if two owners are needed, record the sequence and dispatch only the first one
- log the routing reason in `logs/LOG_DESPACHOS_ORQUESTRADOR.md`
- if the request is still ambiguous after reading the matrix, ask the user instead of guessing
- `@qualidade` validates and gathers evidence; it does not replace `@auditoria` as the approval gate
- after every specialist round, the next two agents are mandatory unless there is a hard blocker:
  1. `@qualidade`
  2. `@auditoria`
- for documentation-only or non-runtime rounds, `@qualidade` may produce a short `nao_aplicavel` validation note, but it still must be called

## Audit-loop ownership

The orchestrator owns the acceptance loop.

This means:
- the orchestrator must call `@qualidade`
- the orchestrator must call `@auditoria`
- the orchestrator must interpret the QA result first
- the orchestrator must interpret the audit result
- if QA is not approved, the orchestrator must extract the gaps
- those QA gaps must become the next specialist dispatch
- if the audit is not approved, the orchestrator must extract the gaps
- those gaps must become the next specialist dispatch
- after the fix, the orchestrator must call `@qualidade` again and then `@auditoria` again

Do not leave the audit loop implicit or assume another agent will close it.

When the audit or QA finding is:
- reproducible
- within the current scope
- actionable without a hidden product decision

the orchestrator must:
- reopen the module automatically
- dispatch the next specialist automatically
- dispatch `@qualidade` automatically after the specialist round
- continue the loop automatically

Do not ask the user for permission to continue in this case.

## State model

Use the state model in [references/state-model.md](references/state-model.md).

Always track these dimensions separately:
- `PRD funcional`
- `PRD infraestrutura`
- `Status geral do modulo`
- `Necessita nova rodada de agentes agora`
- `Gatilho de reabertura`
- `Arquivo(s) ou artefato(s) acompanhados na rodada`
- `Arquivo(s) realmente alterado(s)`
- `Evidencias de validacao`

Do not collapse all of that into a single vague status.

## Delegation rules

- Do not call specialist agents again if the central tracker says `Necessita nova rodada de agentes agora: nao` unless:
  - the user expanded scope
  - the user explicitly asked to reopen the module
  - code or docs changed after the last audit approval
  - the tracker itself says the module must be reopened
- If the tracker and module history disagree, fix the traceability first, then delegate.
- If a module is already approved, report that status instead of re-running the whole loop.

## Required outputs from orchestration

Every orchestration round must leave:
- the module history updated
- `docs/RASTREABILIDADE_PRDS.md` updated
- `logs/LOG_DESPACHOS_ORQUESTRADOR.md` updated when specialist agents were dispatched
- QA evidence recorded as `aprovado`, `nao_aplicavel`, or `gaps reenviados`
- the audit result recorded as either `aprovado` or `gaps reenviados`
- the history entry naming the target PRD
- the history entry naming the files or artifacts tracked in that round
- the history entry naming the files actually changed
- the history entry naming the validation evidence
- the next action explicit as one of:
  - `nenhuma`
  - `chamar @produto`
  - `chamar @infraestrutura`
  - `chamar @mongodb`
  - `chamar @frontend`
  - `chamar @backend`
  - `chamar @qualidade`
  - `chamar @auditoria`
  - `aguardar usuario`

## Stop conditions

Stop only when one of these is true:
- `@auditoria` approved and the tracker says no new round is needed
- there is a real blocker or missing user decision
- the remaining work is explicitly out of scope
