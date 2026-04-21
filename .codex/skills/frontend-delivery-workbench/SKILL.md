---
name: frontend-delivery-workbench
description: Desenvolver e evoluir frontends com foco em componentes, paginas, fluxos, estado, responsividade, acessibilidade, integracao com APIs e prontidao de entrega. Usar quando Codex precisar implementar telas, componentes React, fluxos de formulario, rotas, consumo de API, estados de carregamento/erro, refinamento de UX, documentacao de frontend ou correcoes estruturais, sempre respeitando a arquitetura do projeto e validando a experiencia final.
---

# Frontend Delivery Workbench

Usar esta skill para conduzir mudancas de frontend do inicio ao fim. A ideia e evitar implementacao solta: primeiro entender a stack e o contrato estrutural, depois implementar no lugar certo, e por fim validar comportamento, UX e prontidao de entrega.

## Fluxo obrigatorio

1. Inspecionar o frontend
- Executar `python <skill-dir>/scripts/inspect_frontend_project.py --project-root <repo>`.
- Descobrir framework, runtime, package manager, arquitetura existente, roteamento, estado, testes, estilos e docs relevantes.
- Procurar primeiro por `docs/FRONTEND_ARCHITECTURE.md`, `FRONTEND_ARCHITECTURE.md`, `docs/ARCHITECTURE.md` ou equivalente.
- Se o projeto for React e a skill `react-project-architecture-guardian` estiver disponivel, le-la antes de mudancas estruturais.

2. Delimitar a mudanca
- Identificar se a solicitacao mexe em:
  - layout ou navegacao
  - componente ou pagina
  - formulario e validacao
  - estado local ou global
  - consumo de API
  - acessibilidade, responsividade ou polimento visual
- Ler `references/frontend-delivery-standard.md` para aplicar o checklist correto.

3. Implementar na camada correta
- Colocar layout global, roteamento e providers no entrypoint certo do frontend.
- Colocar componentes reutilizaveis fora de features especificas.
- Colocar regras de negocio do frontend, orquestracao de estado e chamadas de API nos modulos adequados, sem espalhar fetch e logica arbitrariamente em qualquer componente.
- Evitar componentes gigantes misturando renderizacao, logica, efeitos e acesso remoto sem necessidade.

4. Coordenar arquitetura e design
- Se a mudanca for estrutural, priorizar o contrato arquitetural.
- Se a mudanca exigir refinamento visual forte e a skill `frontend-design` estiver disponivel, usa-la depois que a estrutura estiver definida.
- Design nao pode violar a arquitetura; arquitetura nao deve justificar UX ruim quando houver espaco para ajustar o componente.

5. Validar comportamento e experiencia
- Cobrir no minimo:
  - loading, empty, error e success states quando aplicavel
  - responsividade desktop e mobile
  - acessibilidade basica: rotulos, semantica, foco e contraste quando aplicavel
  - integracao com API ou mocks equivalentes
- Se a mudanca envolver fluxo importante de interface e a skill `playwright-feature-qa-auditor` estiver disponivel, considera-la para QA navegador-real.

6. Atualizar testes e documentacao
- Adicionar ou ajustar testes no nivel certo:
  - unidade para hooks, utilitarios e componentes puros
  - integracao para fluxos de pagina, estado e chamadas de API
  - e2e quando o fluxo justificar e a stack suportar
- Atualizar docs, exemplos ou notas de uso quando o comportamento mudar.

7. Verificar antes de encerrar
- Rodar testes, lint, type-check e build relevantes quando existirem.
- Confirmar que a mudanca ficou no modulo certo, com impacto conhecido.
- Responder com o que mudou, o que foi validado e os gaps ou riscos restantes.

## Regras de qualidade

- Nao entregar UI sem estados de carregamento e erro quando eles forem necessarios.
- Nao criar componente reutilizavel preso a uma feature sem necessidade.
- Nao introduzir fetch, estado global ou side effect em local errado por conveniencia.
- Declarar premissas quando faltarem informacoes.
- Equilibrar estrutura, usabilidade e polimento visual.

## Baseline recomendado

- contrato frontend em `docs/FRONTEND_ARCHITECTURE.md` ou equivalente
- componentes compartilhados separados de features
- integracoes de API isoladas em servicos, hooks ou camada equivalente
- testes proporcionais ao risco da mudanca
- validacao minima de responsividade e acessibilidade

## Recursos

### `scripts/inspect_frontend_project.py`

Inspecionar o repositorio para detectar stack frontend, arquitetura, estilos, testes e pontos de entrega.

### `references/frontend-delivery-standard.md`

Consultar o fluxo de entrega, os checklists por tipo de mudanca e as verificacoes finais de frontend.
