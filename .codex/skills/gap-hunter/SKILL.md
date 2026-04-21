---
name: gap-hunter
description: Encontrar e corrigir gaps tecnicos de robustez em aplicacoes (validacoes ausentes, fluxos incompletos, erros nao tratados, edge cases, inconsistencias front/back, contratos frageis e regressao potencial), sempre com ajuste de testes quando aplicavel e relatorio versionado em gap_reports/. Usar quando o usuario pedir buscar gaps, auditar aplicacao, procurar falhas, qa, sanity, verificar fluxos, encontrar edge cases, hardening ou robustez.
---

# Gap Hunter

## Objetivo

Executar auditoria tecnica ponta a ponta para encontrar lacunas reais de robustez, corrigir os problemas identificados e deixar o projeto validado.
Nao encerrar em diagnostico: implementar correcao e validar ate ficar verde.

## Preparacao obrigatoria

1. Ler documentacao do repositorio para entender o fluxo funcional esperado: `README*`, `docs/indice.md`, requisitos e documentos equivalentes.
2. Mapear fluxos criticos no codigo: frontend, backend, configs, schemas, rotas, validacoes e integracoes.
3. Definir escopo auditado (areas, arquivos e fluxos) e registrar no relatorio final.

## Descoberta de comandos oficiais

1. Descobrir comandos reais no repositorio (`package.json`, `Makefile`, `pyproject.toml`, scripts do projeto).
2. Usar apenas comandos existentes.
3. Nao inventar comandos, flags ou pipelines.

## Validacoes obrigatorias

Rodar tudo que existir no projeto para:

1. `lint` e/ou `format`
2. `test`
3. `typecheck` e/ou `build`
4. `smoke` (se existir)

Registrar no relatorio cada comando executado e o resultado (passou/falhou e resumo do erro quando falhar).

## Checklist obrigatorio de gaps

Cobrir explicitamente todas as categorias abaixo:

1. Validacao de entrada ausente (frontend e backend)
2. Tratamento de erro incompleto (`try/catch`, fallback, mensagens)
3. Estados de loading/empty/error inconsistentes (UI)
4. Contratos/API: campos opcionais vs obrigatorios, breaking changes, status codes
5. Autenticacao/autorizacao ausente em rotas sensiveis
6. Concorrencia/race conditions (requests duplicadas, idempotencia)
7. Null/undefined/empty data (edge cases)
8. Paginacao/filtros/ordenacao com comportamento incorreto
9. Timeouts/retries/circuit breaker (quando aplicavel)
10. Logs/observabilidade insuficiente em pontos criticos (quando aplicavel)

## Loop obrigatorio de execucao

Executar em iteracoes curtas, corrigindo 1 gap por vez:

1. Identificar fluxo critico com base em docs + codigo.
2. Rodar validacoes disponiveis para estabelecer baseline.
3. Procurar gaps pelas categorias do checklist.
4. Corrigir 1 gap identificado.
5. Adicionar ou ajustar teste para cobrir o gap corrigido, quando aplicavel.
6. Re-rodar validacoes relevantes e depois o pacote completo.
7. Repetir ate nao haver gaps criticos/medios evidentes e o projeto estar verde.

## Regras de correcao e testes

1. Sempre corrigir gaps encontrados; nao apenas reportar.
2. Priorizar mudancas pequenas por iteracao, mantendo comportamento esperado fora do escopo do bug.
3. Ao corrigir um gap testavel, sempre criar/ajustar teste (unit/integration conforme padrao existente no repo).
4. Se um gap nao puder ser coberto por teste automatizado, justificar tecnicamente no relatorio.

## Relatorio versionado obrigatorio

1. Garantir pasta `gap_reports/` no root do repositorio.
2. Garantir que `gap_reports/` nao esteja ignorado pelo git; ajustar `.gitignore` com o minimo necessario:

```gitignore
!gap_reports/
!gap_reports/**
```

3. Gerar 1 arquivo por execucao:
`gap_reports/YYYY-MM-DD_HH-mm_gap-hunter.md`

4. Incluir no relatorio:
- Escopo auditado (areas/arquivos/fluxos)
- Checklist executado (categorias de gaps)
- Comandos executados e resultados
- Gaps encontrados com severidade (`CRITICO`, `MEDIO`, `BAIXO`)
- Correcao aplicada para cada gap (o que mudou)
- Testes adicionados/ajustados
- Arquivos alterados/criados
- Riscos restantes e proximos passos

## Formato de saida obrigatorio da execucao

Entregar sempre:

1. Resumo do que foi auditado
2. Lista de gaps encontrados e corrigidos
3. Lista de comandos que passaram
4. Nome do arquivo gerado em `gap_reports/`
