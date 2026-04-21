---
name: diagram-mermaid
description: Gerar e manter diagramas Mermaid com base no contexto real do projeto, analisando codigo, configuracoes e documentacao sem inventar entidades. Salvar diagramas em docs/diagrams/, manter docs/db/ para artefatos de banco e atualizar docs/indice.md com links. Use quando o usuario pedir mermaid, diagrama, fluxo, sequencia, arquitetura, estado, er, uml, diagramar ou desenhar fluxo.
---

# Diagram Mermaid

## Overview

Gerar diagramas Mermaid validos e coerentes com o que existe no repositorio.
Atualizar navegacao em `docs/indice.md` para manter rastreabilidade dos diagramas gerados.

## Regras Obrigatorias

1. Verificar `docs/` no root; criar `docs/` e `docs/indice.md` se ausentes.
2. Garantir existencia de duas pastas separadas:
- `docs/diagrams/` para arquivos Mermaid (`.mmd`) e renders (`.png/.svg`)
- `docs/db/` para DBML e relatorios de banco
3. Salvar todo diagrama Mermaid em `.mmd` dentro de `docs/diagrams/`.
4. Usar nomenclatura padrao:
- `docs/diagrams/YYYY-MM-DD_<slug>_<tipo>.mmd`
5. Incluir no topo de cada `.mmd`:
- `%% data: YYYY-MM-DD`
- `%% tipo: <flowchart|sequenceDiagram|classDiagram|stateDiagram-v2|erDiagram|gantt>`
- `%% fontes: <lista curta de paths analisados>`
- `%% escopo: <1 linha do que o diagrama cobre>`
6. Analisar o projeto sem gaps:
- Ler markdown existente (`README`, `docs/`, notas)
- Analisar codigo e configuracoes (`src/`, endpoints, services, jobs, configs)
- Analisar banco (migrations/schema/ORM models) quando o tipo pedido exigir
7. Nao inventar entidades, fluxos ou relacionamentos:
- Usar apenas fatos confirmados no repo
- Marcar lacunas como `%% TODO:` no `.mmd` quando faltarem evidencias
8. Atualizar sempre `docs/indice.md`:
- Criar secao `Diagramas (Mermaid)` se ausente
- Listar e linkar todos os `.mmd` de `docs/diagrams/`
- Ordenar por data desc
- Incluir 1 descricao curta por diagrama
9. Validar Mermaid:
- Se houver comando/ferramenta no repo (por exemplo `mmdc`, script no `package.json`, `Makefile`), executar e registrar comando real
- Se nao houver, executar validacao minima de sintaxe/consistencia
10. Gerar PNG/SVG ao lado do `.mmd` apenas quando houver ferramenta existente no repo.
11. Nao alterar `.gitignore` de forma ampla:
- Ajustar somente o minimo quando `docs/diagrams/` estiver sendo ignorado
- Regras permitidas: `!docs/diagrams/` e `!docs/diagrams/**`

## Schema Do Arquivo .mmd (Obrigatorio)

Seguir sempre a ordem:

1. Header (comentarios)
2. Codigo Mermaid valido
3. Notas finais em comentario (somente se necessario), incluindo `%% TODO:` para lacunas confirmadas

## Tipos Suportados

- `flowchart` (`flowchart TD` ou `flowchart LR`)
- `sequenceDiagram`
- `classDiagram`
- `stateDiagram-v2`
- `erDiagram`
- `gantt` (quando fizer sentido)

## Processo (Sempre Seguir)

1. Entender o pedido e escolher o tipo de diagrama adequado.
2. Mapear entidades reais do repo:
- modulos, services, controllers, endpoints, filas/jobs
- tabelas e relacionamentos quando aplicavel
3. Gerar o `.mmd` com clareza, baixo ruido e fidelidade ao codigo.
4. Validar com comando real existente ou validacao minima.
5. Atualizar `docs/indice.md` com link e descricao.
6. Retornar evidencias no chat conforme formato obrigatorio.

## Formato De Saida Obrigatorio

Sempre retornar:

- `Arquivo(s) criado(s)/alterado(s)`
- `Resumo do diagrama` (1 a 5 bullets)
- `Checklist de validacao`
- `Proximos passos` (somente se houver `TODOs`)

Incluir tambem:

- caminho do `.mmd` criado
- caminho do `.png/.svg` quando gerado
- checklist do que foi analisado (markdown + codigo + configs)
- comando real de validacao/render quando existir

Se nao houver ferramenta de render, declarar explicitamente que foi feita validacao minima de sintaxe.
