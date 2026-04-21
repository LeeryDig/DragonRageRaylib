---
name: schema-dbml
description: Gerar e manter schema DBML a partir do contexto real do projeto, analisando codigo, configuracoes e docs sem inventar tabelas, colunas ou relacionamentos. Salvar DBML em docs/db/, atualizar docs/indice.md, registrar relatorio tecnico de decisoes e inconsistencias, e priorizar migrations como fonte de verdade quando existirem. Use quando o usuario pedir dbml, schema, modelo de dados, banco, tabelas, relacionamentos, erd, diagrama de banco ou gerar db.
---

# Schema DBML

## Overview

Generate and maintain DBML that reflects the real database model from repository evidence.
Keep DB documentation organized in `docs/db/` and indexed in `docs/indice.md`.

## Regras Obrigatorias

1. Verificar `docs/`; se nao existir, criar `docs/` e `docs/indice.md`.
2. Garantir sempre duas pastas separadas:
- `docs/diagrams/` para Mermaid e renders
- `docs/db/` para DBML e relatorios de banco
3. Salvar schema DBML em:
- `docs/db/schema.dbml` (padrao)
- ou `docs/db/YYYY-MM-DD_schema.dbml` (quando versao por data for necessaria)
4. Analisar projeto sem gaps:
- Markdown (`README`, `docs/`, notas)
- Codigo e configuracoes
- Fontes de schema: migrations, schema files, ORM models, SQL confiavel
5. Definir fonte de verdade com prioridade obrigatoria:
- Se houver migrations, usar migrations como fonte principal
- Se nao houver migrations, usar schema files e/ou models ORM
- Em divergencia, documentar e escolher a fonte mais confiavel (geralmente migrations)
6. Nunca inventar tabelas, colunas ou relacionamentos.
7. Se faltar informacao, registrar `TODO` no DBML com o que falta e onde procurar.
8. Respeitar nomes reais do repositorio (sem traduzir/renomear arbitrariamente).
9. Atualizar `docs/indice.md`:
- Criar secao `Banco de Dados (DBML)` se ausente
- Linkar `docs/db/schema.dbml` (ou arquivo gerado)
- Incluir descricao curta
10. Validar DBML:
- Se existir ferramenta/comando no repo, executar e registrar comando real
- Se nao existir, validar estrutura DBML minima (`Table`, `Ref`, FKs para colunas existentes)
11. Nao alterar `.gitignore` de forma ampla.
12. Se `docs/db/` estiver ignorado, ajustar somente:
- `!docs/db/`
- `!docs/db/**`

## Relatorio Tecnico Obrigatorio

Gerar em `docs/db/`:
- `docs/db/dbml_report.md` ou `docs/db/YYYY-MM-DD_dbml_report.md`

O relatorio deve conter:
- Fontes analisadas (paths)
- Fonte de verdade escolhida e justificativa
- Divergencias encontradas
- Decisoes tomadas (nomes, tipos, relacionamentos)
- Proximos passos (`TODOs`)

## Processo (Sempre Seguir)

1. Detectar stack e localizar fontes de verdade (`migrations`, `schema`, `models`).
2. Mapear tabelas, colunas, tipos, PKs, FKs e indexes quando possivel.
3. Gerar `schema.dbml` com:
- `Table` para cada tabela real
- colunas e tipos observados
- chaves e `Ref` de relacionamentos
- `Note`/comentarios para observacoes importantes
4. Validar DBML com comando existente no repo, ou validacao minima estrutural.
5. Corrigir inconsistencias possiveis com mudanca minima e documentar no relatorio.
6. Atualizar `docs/indice.md` e gerar `dbml_report.md`.
7. Retornar resultados no chat no formato obrigatorio.

## Evidencia Minima De Analise

Sempre evidenciar no trabalho:
- Markdown analisado
- Codigo/configs analisados
- Migrations/schema/models analisados

Sem essa triangulacao, o DBML nao deve ser considerado concluido.

## Formato De Saida Obrigatorio

Sempre retornar:

- `Arquivo(s) criado(s)/alterado(s)`
- `Resumo do que foi encontrado` (tabelas principais e relacionamentos principais)
- `Checklist de validacao`
- `Proximos passos` (somente se houver `TODOs`)

Tambem incluir:

- caminho do DBML gerado
- caminho do relatorio gerado
- checklist do que foi analisado (markdown + codigo + migrations/models)
- comando real de validacao, quando existir
