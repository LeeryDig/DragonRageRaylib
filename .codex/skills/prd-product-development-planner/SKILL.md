---
name: prd-product-development-planner
description: Ler um PRD quando ele existir e transformar esse insumo em um plano de desenvolvimento do produto com fases, checklist de verificacao, entregaveis, riscos, dependencias e responsabilidades continuas de gerente de projeto. Usar quando Codex precisar planejar a execucao de um produto, organizar roadmap de implementacao, estruturar milestones, definir gates de verificacao ou montar o acompanhamento operacional de um projeto a partir de PRD, README, docs de produto ou documentos equivalentes.
---

# PRD Product Development Planner

Usar esta skill para converter um PRD ou documento equivalente em um plano acionavel de execucao. Sempre procurar o PRD primeiro; se ele nao existir, usar documentos proximos como fallback e deixar as lacunas explicitamente marcadas.

## Fluxo obrigatorio

1. Localizar a fonte principal
- Executar `python <skill-dir>/scripts/discover_prd.py --project-root <repo>`.
- Se houver um PRD claro, usar esse arquivo como fonte primaria.
- Se nao houver PRD, escolher o melhor substituto entre README, docs de produto, especificacoes ou notas funcionais e declarar no plano que o PRD nao foi encontrado.
- Nao fingir certeza quando houver lacunas. Criar uma secao de premissas e perguntas em aberto.

2. Extrair o nucleo do produto
- Identificar objetivo do produto, problema resolvido, personas, escopo, fora de escopo, requisitos funcionais, requisitos nao funcionais, restricoes, dependencias, riscos e criterio de sucesso.
- Ler `references/planning-framework.md` para padronizar a estrutura do plano e o checklist gerencial.

3. Montar o plano de desenvolvimento
- Produzir um plano com fases claras. Usar, no minimo:
  - alinhamento e descoberta
  - detalhamento e preparacao
  - implementacao
  - validacao e go-live
  - pos-lancamento
- Para cada fase, incluir:
  - objetivo
  - entregaveis
  - checklist de verificacao
  - donos sugeridos
  - dependencias
  - riscos principais
  - criterio de conclusao

4. Incluir a rotina do gerente de projeto
- Adicionar as atividades continuas de PM ao longo do projeto:
  - alinhamento de stakeholders
  - gestao de riscos
  - acompanhamento de cronograma
  - refinamento de escopo
  - remocao de bloqueios
  - comunicacao de status
  - validacao de readiness para release
  - acompanhamento pos-lancamento
- Separar atividades recorrentes das atividades por fase.

5. Fechar com governanca e verificacao
- Adicionar checklist final de readiness, marcos, dependencias externas e decisoes pendentes.
- Se faltarem dados no PRD ou nos documentos, destacar isso como risco de planejamento.
- Se o usuario pedir cronograma, transformar as fases em sequencia com ordem, paralelismo e prioridade; nao inventar datas sem base.

## Regras de qualidade

- Preferir clareza operacional a texto generico.
- Declarar explicitamente a fonte usada: PRD ou fallback.
- Diferenciar fato, inferencia e premissa.
- Nao misturar backlog detalhado com plano executivo sem rotular.
- Se o plano ficar grande, priorizar faseamento, gates e checklist.

## Estrutura minima de saida

- contexto e fonte
- resumo executivo
- objetivos e sucesso
- fases do projeto
- checklist de verificacao por fase
- atividades recorrentes do gerente de projeto
- riscos, dependencias e bloqueios
- decisoes em aberto

## Recursos

### `scripts/discover_prd.py`

Localizar arquivos candidatos a PRD e documentos equivalentes no projeto para orientar a escolha da fonte principal.

### `references/planning-framework.md`

Consultar a estrutura recomendada do plano, os gates por fase e o checklist continuo do gerente de projeto.
