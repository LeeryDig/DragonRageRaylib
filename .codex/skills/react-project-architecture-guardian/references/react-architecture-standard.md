# React Architecture Standard

Usar esta referencia para criar ou revisar `docs/FRONTEND_ARCHITECTURE.md`.

## Objetivo

Definir um contrato simples e executavel para projetos React. O foco e manter clareza entre entrypoints, features, UI reutilizavel, hooks, servicos e politica de runtime.

## Politica de Node.js

- Antes de recomendar ou alterar a versao do Node, verificar a fonte oficial da Node.js.
- Usar por padrao a `Latest Release`, nao a `Latest LTS`, salvo pedido explicito em contrario.
- Atualizar todos os pontos de versao juntos:
  - `package.json` em `engines.node`
  - `.nvmrc`
  - `.node-version`
  - workflows GitHub com `actions/setup-node`
  - `Dockerfile` baseado em `node:`
  - documentacao relevante
- No momento da criacao desta skill, a pagina oficial de download mostrava `v25.9.0` como `Latest Release` e `v24.14.1` como `Latest LTS` em 1 de abril de 2026, mas essa verificacao deve ser refeita sempre.

## Baseline recomendado

### Para React com Vite ou SPA

- `src/app/`
  - bootstrap, providers, configuracao global, roteamento
- `src/features/`
  - blocos de negocio do frontend por dominio funcional
- `src/components/`
  - UI reutilizavel e componentes compartilhados
- `src/hooks/`
  - hooks compartilhados ou por dominio, sem mistura arbitraria
- `src/services/`
  - clientes HTTP, adaptadores, gateways externos
- `src/lib/`
  - utilitarios puros, formatadores, funcoes sem acoplamento com React
- `src/styles/`
  - estilos globais, tokens, temas
- `tests/`
  - integracao, e2e ou utilitarios de teste

### Para Next.js

- `app/` ou `pages/`
  - entrypoints do framework, layouts, rotas e composicao de tela
- `src/features/`, `src/components/`, `src/hooks/`, `src/services/`, `src/lib/`, `src/styles/`
  - mesma logica de responsabilidade do baseline SPA
- Evitar colocar regra de negocio dispersa dentro de arquivos de rota quando ela puder morar em `features` ou `services`.

## Regras de dependencia

- `app/`, `app/` do Next ou `pages/` podem depender de `features`, `components`, `hooks`, `services` e `lib`.
- `features/` podem depender de `components`, `hooks`, `services` e `lib`.
- `components/` compartilhados nao devem depender de `features/`.
- `lib/` nao deve depender de `features/`, `pages/` ou entrypoints.
- `services/` devem concentrar integracoes externas; componentes nao devem espalhar chamadas HTTP arbitrarias.

## Regras praticas

- Preferir TypeScript em projetos novos.
- Evitar componente gigante que faz roteamento, fetch, estado complexo e renderizacao ao mesmo tempo.
- Evitar store global sem justificativa clara.
- Colocar codigo novo no modulo mais proximo da responsabilidade real.
- Fazer testes refletirem a estrutura principal.

## Relacao com `frontend-design`

- `frontend-design` pode definir visual, movimento e refinamento de interface.
- Esta skill define onde o codigo vive e como as dependencias se organizam.
- O design nao deve quebrar o contrato estrutural.

## Template de `docs/FRONTEND_ARCHITECTURE.md`

```md
# Frontend Architecture

## Objetivo
- O que esta arquitetura precisa otimizar?

## Stack e runtime
- Framework:
- Package manager:
- Node.js alvo:
- Motivo da escolha:

## Estrutura do repositorio
- Onde ficam app, features, componentes, hooks, servicos, estilos e testes?

## Responsabilidades por pasta
- `src/app`:
- `src/features`:
- `src/components`:
- `src/hooks`:
- `src/services`:
- `src/lib`:
- `src/styles`:

## Regras de dependencia
- Dependencias permitidas:
- Dependencias proibidas:

## Convencoes de mudanca
- Onde adicionar codigo novo
- Quando criar nova pasta
- Quando atualizar este documento

## Politica de Node.js
- Fonte oficial consultada:
- Versao oficial adotada:
- Arquivos que precisam ficar sincronizados:
```

## Checklist de conformidade

- O contrato frontend existe e esta atualizado.
- A versao do Node esta alinhada em todos os arquivos relevantes.
- Cada pasta tem responsabilidade clara.
- Dependencias seguem a direcao definida.
- Entry points ficam nas bordas.
- Mudancas estruturais atualizam o contrato.
