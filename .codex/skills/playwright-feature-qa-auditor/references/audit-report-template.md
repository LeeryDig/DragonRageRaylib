# Audit Report Template

Usar este arquivo como referencia rapida para preencher os artefatos da auditoria.

## Objetivo

- Validar uma funcionalidade por vez.
- Registrar resultado de execucao real no navegador.
- Deixar claro o que foi coberto, o que falhou e o que ficou bloqueado.

## Estrutura minima de `report.md`

```md
# Auditoria de QA

## Escopo
- Feature:
- Objetivo:
- Ambiente ou URL:
- Data:
- Responsavel:

## Contexto
- Branch ou commit:
- Credenciais ou dados de teste:
- Premissas:
- Fora de escopo:

## Cenarios executados
| ID | Cenario | Resultado | Evidencia |
| --- | --- | --- | --- |
| C-01 | | PASSOU/FALHOU/BLOQUEADO | artifacts/... |

## Resumo executivo
- Status geral:
- Bugs encontrados:
- Bloqueios:
- Riscos residuais:

## Cobertura e gaps
- Coberto:
- Nao coberto:
- Proximos passos:
```

## Estrutura minima de `findings.md`

```md
# Findings

## F-001 - Titulo curto
- Severidade: critica/alta/media/baixa
- Estado: reproduzido/intermitente/bloqueado
- Pre-condicao:
- Passos para reproduzir:
  1.
  2.
  3.
- Resultado esperado:
- Resultado observado:
- Evidencia: artifacts/...
- Impacto:
- Observacao:
```

## Regra de severidade

- `critica`: impedir o uso da funcionalidade principal ou gerar risco grave.
- `alta`: quebrar um fluxo importante com workaround ruim ou inexistente.
- `media`: degradar o fluxo com workaround aceitavel.
- `baixa`: problema cosmetico, textual ou de baixo impacto operacional.

## Checklist de encerramento

- Confirmar se cada defeito foi reproduzido mais de uma vez quando possivel.
- Referenciar cada screenshot, trace ou video pelo caminho relativo.
- Separar bug real de bloqueio de ambiente.
- Declarar explicitamente qualquer area nao testada.
