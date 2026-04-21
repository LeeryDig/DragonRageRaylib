---
name: frontend-styleguide-conformance
description: Auditar um frontend inteiro contra o Style guide oficial, confrontando telas, componentes compartilhados, shell, iconografia, tabelas, cards, inputs, botoes, overlays, feedback e responsividade com a documentacao e os contratos reais do sistema. Usar quando Codex precisar comparar o frontend com `frontend-backoffice/docs/STYLE_GUIDE_OFICIAL.md`, procurar drifts visuais ou estruturais, revisar consistencia antes de fechar uma entrega, ou transformar gaps de aderencia em itens acionaveis com referencias de arquivo.
---

# Frontend Style Guide Conformance

Usar esta skill para confrontar o frontend como um todo com o `Style guide` oficial do modulo e devolver um parecer util para implementacao ou auditoria.

O objetivo nao e so dizer "esta parecido". O objetivo e validar aderencia real aos contratos documentados e distinguir:

- o que e contrato oficial do sistema
- o que e apenas demonstracao visual do guide
- o que e drift de implementacao
- o que e gap de documentacao

## Fluxo obrigatorio

1. Ler as fontes de verdade na ordem certa
- Ler `frontend-backoffice/docs/STYLE_GUIDE_OFICIAL.md`.
- Ler `frontend-backoffice/docs/PRD.md`.
- Se existir, ler `docs/frontend/FRONTEND_ARCHITECTURE.md`.
- Para detalhes de implementacao, ler:
  - `frontend-backoffice/src/app/components/StyleGuideView.jsx`
  - `frontend-backoffice/src/app/components/styleguide/styleguide-data.js`
  - `frontend-backoffice/src/app/components/Common.jsx`
  - `frontend-backoffice/src/ui/Icon.jsx`
  - `frontend-backoffice/src/ui/icon-map.js`
  - `frontend-backoffice/public/styles/tokens.css`
  - `frontend-backoffice/public/styles/shell.css`

2. Mapear o frontend real
- Tratar `frontend-backoffice/` como workspace canonico do frontend.
- Identificar as principais superficies reais: shell, menu, breadcrumb, dashboard, repositorio, configuracoes, notificacoes, inspector lateral e componentes compartilhados.
- Usar `rg` para localizar componentes, classes e rotas antes de concluir que um padrao esta ausente.

3. Auditar por contrato, nao por preferencia
- Confrontar a implementacao com o contrato documentado no `Style guide`.
- Nao inventar um novo padrao visual se o sistema ja tiver um contrato oficial.
- Nao elevar um exemplo visual do guide a requisito oficial quando o proprio guide disser que aquilo e apenas demonstracao.
- Para separar contrato de demonstracao, consultar a secao "O que no guide e demonstracao e o que e regra" do markdown oficial.

4. Cobrir o frontend inteiro por categorias
- Executar a auditoria pelas categorias do guide:
  - shell e navegacao
  - menus e entrada por cards
  - tabelas
  - cards
  - inputs e formularios
  - upload customizado
  - selecao operacional
  - botoes e acoes iconicas
  - overlays
  - feedback e toasts
  - chat de referencia
  - graficos
  - QR preview
  - iconografia
  - acessibilidade e responsividade

5. Produzir gaps acionaveis
- Para cada drift encontrado, registrar:
  - severidade
  - superficie afetada
  - contrato quebrado
  - evidencia no codigo
  - correcao esperada
- Sempre referenciar arquivo e, quando possivel, componente ou classe.
- Priorizar gaps que violam contratos compartilhados e se espalham por varias telas.

6. Se o pedido incluir correcao, corrigir e revalidar
- Aplicar a menor mudanca defensavel.
- Reaproveitar componentes e contratos existentes antes de criar novos.
- Depois da correcao, rodar pelo menos build e as validacoes de frontend disponiveis.

## Regras de decisao

- Se o `Style guide` e o codigo divergirem, tratar o codigo real do guide e os contratos compartilhados como fonte principal e sinalizar a documentacao stale.
- Se um padrao estiver no `Style guide`, mas nao existir em nenhuma superficie real, nao forcar rollout global sem pedido explicito; registrar como gap de cobertura ou de adocao.
- Se uma tela estiver melhor que o guide e em linha com os principios oficiais, preferir propor ajuste da documentacao em vez de piorar a tela.
- Se o drift for localizado e nao quebrar contrato compartilhado, classificar como gap pontual, nao como problema sistemico.

## Saida esperada

Quando a skill for usada em modo de auditoria, responder com:

1. resumo executivo curto
2. findings ordenados por severidade
3. categorias auditadas
4. itens que estao aderentes
5. gaps de documentacao, quando existirem

Quando a skill for usada em modo de implementacao, responder com:

1. o que foi ajustado para aderir ao guide
2. quais contratos foram respeitados
3. o que foi validado depois da mudanca
4. riscos ou pontos ainda pendentes

## Referencia complementar

- Ler `references/styleguide-conformance-checklist.md` para o checklist detalhado por categoria e o formato recomendado dos findings.
