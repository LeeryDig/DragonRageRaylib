---
name: react-project-architecture-guardian
description: Definir, documentar e proteger a arquitetura de projetos React, incluindo organizacao de pastas, limites entre componentes e features, padrao de estado, roteamento e politica de versao do Node.js. Usar quando Codex precisar estruturar ou revisar um projeto React, criar ou atualizar um contrato arquitetural em docs/FRONTEND_ARCHITECTURE.md, decidir onde novo codigo frontend deve ficar ou garantir que o projeto siga sempre a ultima versao oficial do Node.js.
---

# React Project Architecture Guardian

Usar esta skill para transformar a arquitetura React em um contrato operacional do projeto. Se houver um contrato frontend, ele deve ser lido primeiro e obedecido. Se nao houver, ele deve ser criado antes de espalhar novos padroes estruturais.

## Fluxo obrigatorio

1. Inspecionar o projeto React
- Executar `python <skill-dir>/scripts/inspect_react_project.py --project-root <repo>`.
- Identificar stack real: React puro, Vite, Next.js, TypeScript, roteamento, testes, package manager, scripts e sinais de arquitetura existente.
- Procurar primeiro por `docs/FRONTEND_ARCHITECTURE.md`, `FRONTEND_ARCHITECTURE.md`, `docs/ARCHITECTURE.md` ou equivalente.

2. Validar a politica do Node.js
- Antes de recomendar ou alterar runtime, verificar a ultima versao oficial do Node.js nas paginas oficiais da Node.js.
- Usar por padrao a `Latest Release`, nao a `Latest LTS`, a menos que o usuario peça LTS explicitamente.
- Sincronizar todos os pontos de versao quando houver mudanca:
  - `package.json` em `engines.node`
  - `.nvmrc`
  - `.node-version`
  - GitHub Actions com `actions/setup-node`
  - `Dockerfile` baseado em `node:`
  - documentacao do projeto
- Nao deixar versoes conflitantes entre arquivos.

3. Definir ou atualizar o contrato arquitetural
- Se nao existir contrato, criar `docs/FRONTEND_ARCHITECTURE.md` usando `references/react-architecture-standard.md` como baseline.
- Se ja existir contrato, preserva-lo e atualiza-lo apenas quando houver necessidade real.
- O contrato deve registrar no minimo:
  - framework e runtime alvo
  - estrutura de pastas
  - responsabilidade de cada camada ou pasta
  - regra de dependencia entre modulos
  - onde componentes, hooks, servicos, estados, estilos e testes devem ficar
  - como introduzir excecoes ou evolucoes

4. Escolher a arquitetura React apropriada
- Preferir simplicidade e coerencia com o tamanho do projeto.
- Para Vite ou SPA React, preferir separar `app`, `features`, `components`, `hooks`, `lib`, `services`, `styles` e `tests` quando o tamanho justificar.
- Para Next.js, respeitar `app/` ou `pages/` como entrypoint do framework e manter a regra de negocio e UI reutilizavel fora dessas bordas quando possivel.
- Preferir TypeScript em projetos novos, salvo pedido explicito para JavaScript puro.
- Evitar estado global sem dono claro e evitar componentes gigantes com regra de negocio misturada.

5. Aplicar a arquitetura em toda mudanca futura
- Antes de editar, mapear a solicitacao para o modulo, camada ou pasta correta segundo o contrato.
- Se a mudanca conflitar com o contrato, pausar e explicar o conflito.
- Nao criar nova pasta raiz, novo padrao de estado ou novo modelo de componentes sem atualizar conscientemente o contrato.
- Se a skill `frontend-design` estiver disponivel, usa-la apenas depois que a arquitetura estiver definida; design nao pode violar o contrato estrutural.

6. Revisar conformidade arquitetural
- Em implementacoes ou reviews, verificar:
  - local correto do codigo
  - direcao correta das dependencias
  - componentes com responsabilidades claras
  - consistencia entre componentes, hooks, servicos e testes
  - alinhamento da versao do Node em todos os arquivos relevantes
- Se encontrar violacao, apontar e sugerir o ajuste mais simples.

## Regras de governanca

- Ler o contrato frontend antes de trabalho estrutural ou refactor.
- Tratar `docs/FRONTEND_ARCHITECTURE.md` como fonte de verdade local para o frontend.
- Tratar a versao oficial mais recente do Node.js como regra padrao do projeto, salvo excecao explicitamente aprovada.
- Se for necessario fugir da arquitetura, pedir aprovacao e atualizar o contrato junto com a mudanca.
- Preferir evolucao incremental a reestruturacao total.

## Baseline recomendado

- `docs/FRONTEND_ARCHITECTURE.md` para o contrato
- `src/app` ou `app/` para bootstrap, providers e roteamento
- `src/features` para blocos de negocio do frontend
- `src/components` para UI reutilizavel
- `src/hooks`, `src/lib`, `src/services`, `src/styles`
- `tests/` ou estrutura equivalente para testes

## Recursos

### `scripts/inspect_react_project.py`

Inspecionar o repositorio para descobrir stack React, package manager, pontos de configuracao do Node e contratos arquiteturais existentes.

### `references/react-architecture-standard.md`

Consultar o baseline arquitetural React, a politica de Node.js e o template para `docs/FRONTEND_ARCHITECTURE.md`.
