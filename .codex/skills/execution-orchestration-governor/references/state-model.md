# State Model

Use these exact meanings.

## Status values

- `planejado`: o PRD existe, mas a rodada ainda nao comecou
- `em_andamento`: existe execucao aberta sem auditoria final
- `em_auditoria`: a rodada foi executada e depende do parecer do `@auditoria`
- `aprovado`: o escopo da rodada foi aceito pelo `@auditoria`
- `bloqueado`: falta decisao, acesso, segredo, ambiente ou escopo
- `fora_da_rodada`: o artefato existe, mas nao esta sendo trabalhado nesta rodada
- `documentado_e_validado`: o PRD ou PRD de infraestrutura foi produzido e validado como planejamento
- `concluido_e_aprovado`: o PRD funcional foi executado e aprovado

## Central tracker fields

For each module, keep:

- `Modulo`
- `PRD funcional`
- `PRD infraestrutura`
- `Status geral`
- `Ultimo validador`
- `Ultima data`
- `Necessita nova rodada de agentes agora`
- `Proximo agente`
- `Reabrir quando`
- `Historico`

## Reopen discipline

Set `Necessita nova rodada de agentes agora: nao` when:
- the last relevant round was approved
- no blocker remains
- there is no explicit user request to reopen

Set `Necessita nova rodada de agentes agora: sim` only when:
- there is an open audit finding
- there is missing implementation against an approved PRD
- there is a blocker requiring another agent
- a new user request expanded the scope
