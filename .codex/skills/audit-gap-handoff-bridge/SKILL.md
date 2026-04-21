---
name: audit-gap-handoff-bridge
description: Transform auditor findings into explicit, traceable gap handoff packets for the orchestrator, so `@auditoria` and `@orquestrador` close the same loop with no ambiguity.
---

# Audit Gap Handoff Bridge

Use this skill whenever `@auditoria` finds a real gap that must be handed to `@orquestrador` for automatic follow-up.

The goal is to stop gaps from living only in prose. Every rejected audit must leave a structured handoff packet that the orchestrator can consume without guessing.

## Core workflow

1. Confirm whether the audit result is:
- `approved`
- `approved_with_risks`
- `rejected_with_gaps`
- `blocked`

2. If the result is `rejected_with_gaps`, create a handoff packet in:
- `logs/AUDITORIA_ORQUESTRADOR_GAPS.md`

3. The packet must be specific enough for the orchestrator to dispatch the next specialist without asking the user again when the owner is obvious.

4. If the result is `approved`, optionally write a short closure packet when that helps traceability, but do not create fake gaps.

5. The orchestrator must read the most recent relevant packet before choosing the next agent.

## Mandatory packet fields

Every rejection packet must include:

- `Modulo`
- `PRD alvo`
- `Status da auditoria`
- `Finding ID`
- `Titulo do gap`
- `Severidade`
- `Reproduzivel`
- `Escopo`
- `Gap objetivo`
- `Evidencias`
- `Arquivos ou artefatos observados`
- `Owner recomendado`
- `Proximo agente`
- `Condicao de aceite para fechar`
- `Pode seguir automaticamente`

## Decision rule

Set `Pode seguir automaticamente` to:

- `sim` when the gap is reproducible, in scope, and has an obvious owner
- `nao` when a user decision, hidden product tradeoff, or blocker still exists

This field is the bridge between `@auditoria` and `@orquestrador`.

## Packet template

Use the template in:

- `references/handoff-packet-template.md`

## Expected behavior by agent

### `@auditoria`

- never stop at "found some issues"
- convert each real rejection into one or more handoff packets
- reference evidence paths, not just summaries
- recommend the next owner explicitly
- treat implicit UI fallbacks that hide missing runtime context as real gaps when the PRD or domain model does not allow them
- do not confuse a backend domain rule with a frontend fallback; if login is canonically derived from CPF in the backend, the client must still not silently invent CPF when runtime context is absent unless the product contract explicitly allows it

### `@orquestrador`

- read the latest packet first
- update `docs/RASTREABILIDADE_PRDS.md`
- update the module `HISTORICO_ANDAMENTO.md`
- update `logs/LOG_DESPACHOS_ORQUESTRADOR.md`
- dispatch the recommended owner when `Pode seguir automaticamente = sim`

## Storage rule

- `logs/AUDITORIA_ORQUESTRADOR_GAPS.md` is operational and local
- keep it outside Git versioning
- do not replace the versioned PRD tracker with this file
- this file complements, not replaces, the tracker and module histories
