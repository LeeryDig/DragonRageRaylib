# Checklist de Confronto com o Style Guide

Usar este checklist para auditar o `frontend-backoffice` inteiro contra `frontend-backoffice/docs/STYLE_GUIDE_OFICIAL.md`.

## 1. Shell e navegacao

Verificar:

- topbar e menu principal
- breadcrumb clicavel
- page header
- entrada por cards em `Configuracoes`
- consistencia entre rotas e contexto visual

Perguntas:

- a macroarea esta no menu certo?
- o breadcrumb acompanha a rota real?
- o card de entrada segue o padrao oficial?

## 2. Tabelas

Verificar:

- casca estrutural
- zebra
- hover
- densidade
- coluna de acoes
- toolbar
- estados de loading, empty, error, selecao, paginacao, linha expandida, erro parcial

Perguntas:

- a tabela segue a familia do `Repositorio`?
- filtro e ordenacao estao representados como backend-first?
- `visualizar`, `editar`, `desativar` e `excluir` estao no overlay correto?

## 3. Cards

Verificar:

- familias `service-card`, `settings-entry-card`, `inspector-card`
- titulo, apoio, borda, espacamento e elevacao
- texto explicativo em cards de `Configuracoes`

Perguntas:

- o card parece do mesmo sistema?
- existe peso visual excessivo?
- o icone esta discreto e coerente?

## 4. Inputs e formularios

Verificar:

- altura
- labels
- helper text
- estados disabled, readonly, success, error, loading
- radio e checkbox
- autocomplete, datepicker, upload customizado

Perguntas:

- os campos estao compactos?
- radio e checkbox parecem leves e alinhados?
- o upload usa o padrao customizado?

## 5. Selecao operacional

Verificar:

- bloco `Escopo de usuarios`
- separacao entre escopo e lista
- CTA depois da escolha
- densidade e respiro

Perguntas:

- a hierarquia do bloco esta clara?
- o CTA esta perto demais?
- a lista de usuarios respira adequadamente?

## 6. Botoes e acoes iconicas

Verificar:

- familias `primary`, `secondary`, `quiet`, `danger`, `split`
- mesma altura e familia
- hover
- separacao entre botao textual e acao iconica

Perguntas:

- a variacao esta vindo de cor/icone e nao de altura?
- ha acao iconica virando botao estranho?
- os icones usados batem com `Icon.jsx` e `icon-map.js`?

## 7. Overlays

Verificar:

- sidebar lateral
- modal de confirmacao
- dropdown
- tooltip
- popover

Perguntas:

- leitura, criacao e edicao usam lateral?
- exclusao usa modal central?
- o overlay esta competindo com o workspace?

## 8. Feedback

Verificar:

- inline alert
- banner persistente
- screen state
- toast

Perguntas:

- a mensagem curta virou toast?
- ha flash grande fora do padrao?
- o estado de tela esta coerente com o contrato?

## 9. Chat, graficos e QR

Verificar:

- chat como referencia, nao fluxo principal
- tipografia dos graficos
- centralizacao e respiro do QR

Perguntas:

- o chat esta so em contexto permitido?
- os graficos estao com fonte pequena e leitura limpa?
- o QR esta centralizado e com respiro acima?

## 10. Acessibilidade e responsividade

Verificar:

- foco visivel
- labels e `aria-*` quando aplicavel
- contraste
- comportamento em desktop, tablet e mobile

Perguntas:

- a tela continua legivel em narrow widths?
- ha alvo interativo pequeno demais?
- existe dependencia exclusiva de cor para comunicar estado?

## 11. Gaps de documentacao

Registrar quando:

- o `Style guide` documenta algo que o codigo nao segue mais
- o frontend implementa um padrao novo ainda nao documentado
- ha nomes, aliases ou contratos stale

## 12. Formato recomendado dos findings

Para cada finding, registrar:

- severidade: `alto`, `medio`, `baixo`
- categoria: ex. `tabelas`, `botoes`, `iconografia`
- superficie: tela, componente ou fluxo
- contrato quebrado: regra do `Style guide`
- evidencia: arquivo(s) afetados
- correcao esperada: o menor ajuste defensavel
