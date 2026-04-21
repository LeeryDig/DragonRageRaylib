# Agent Routing Matrix

Use this matrix before each orchestration round. The `@orquestrador` must decide the next specialist owner explicitly instead of improvising.

## Core rule

Choose one primary next owner first. If more than one specialist will be needed, record the sequence and dispatch only the first one.

After the primary specialist round, the mandatory tail of the sequence is:

1. `@qualidade`
2. `@auditoria`

This applies even when the specialist round was small. For documentation-only or non-runtime work, `@qualidade` may return a short `nao_aplicavel` validation note, but it still must be called.

## Routing matrix

| Situation | Primary next agent | When to sequence another agent after it |
| --- | --- | --- |
| PRD ausente, desatualizado, contraditorio ou sem criterio de aceite | `@produto` | depois chamar `@qualidade` e `@auditoria` |
| Docker, docker-compose, Terraform, GitHub Actions, Azure, variaveis de ambiente, deploy | `@infraestrutura` | depois chamar `@qualidade` e `@auditoria` |
| Modelagem MongoDB, ownership de collections, indices, DBML, fronteira entre modulos | `@mongodb` | depois chamar `@qualidade` e `@auditoria` ou o implementador correspondente antes deles |
| Endpoint, contrato de API, regra de negocio, validacao server-side, persistencia, migration, testes Python de backend | `@backend` | depois chamar `@qualidade` e `@auditoria` |
| Layout, styling, componente, fluxo visual, formulario, copy, consumo de API no cliente, validacao client-side | `@frontend` | depois chamar `@qualidade` e `@auditoria` |
| QA em browser real, smoke, regressao visual, evidencia de funcionamento, reproduzir bug na UI | `@qualidade` | depois chamar `@auditoria` |
| Revisao final de aderencia ao PRD ou aceite da entrega | `@auditoria` | se rejeitar, usar o handoff e voltar ao owner indicado |

## Common composite flows

### Fluxo de produto para implementacao

- `@produto`
- `@qualidade`
- `@auditoria`
- `@backend` ou `@frontend` ou `@infraestrutura` ou `@mongodb`
- `@qualidade`
- `@auditoria`

### Fluxo de gap tecnico encontrado pela auditoria

- `@auditoria`
- ler `logs/AUDITORIA_ORQUESTRADOR_GAPS.md`
- despachar `@backend` ou `@frontend` ou `@infraestrutura` ou `@mongodb`
- `@qualidade`
- `@auditoria`

### Fluxo com implementacao e evidencia em browser

- `@backend` ou `@frontend`
- `@qualidade`
- `@auditoria`

### Fluxo padrao minimo do orquestrador

- especialista dono da rodada
- `@qualidade`
- `@auditoria`

## Hard rules

- `@qualidade` nao aprova PRD; so valida e coleta evidencia obrigatoria antes do gate de auditoria
- `@auditoria` nao substitui `@produto` para redefinir escopo
- `@orquestrador` nao deve escolher implementacao local quando um owner claro existir nesta matriz
- `@orquestrador` nao deve pular `@qualidade` entre o especialista e `@auditoria`
- se o caso envolver mais de um modulo e mais de um tipo de trabalho, escolher primeiro pelo gargalo atual, nao pelo escopo total
- se a ambiguidade for sobre modulo alvo, perguntar ao usuario
- se a ambiguidade for so sobre sequencia entre owners claros, registrar a sequencia e despachar o primeiro
