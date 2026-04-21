---
name: python-project-architecture-guardian
description: Definir, documentar e proteger uma arquitetura Python simples, especialmente para MVPs, APIs pequenas e monolitos modulares. Usar quando Codex precisar organizar um projeto Python sem exagerar em camadas, criar ou atualizar docs/ARCHITECTURE.md, decidir onde novo codigo deve ficar em uma estrutura enxuta com modulos como `main.py`, `routes.py`, `service.py` e `repository.py`, ou garantir que o projeto siga um padrao simples e evolutivo.
---

# Python Project Architecture Guardian

Usar esta skill para transformar a arquitetura Python em um contrato operacional simples e proporcional ao tamanho do projeto. A regra principal e: se houver um contrato arquitetural no projeto, le-lo primeiro e obedecer; se nao houver, defini-lo antes de espalhar novas estruturas.

## Fluxo obrigatorio

1. Inspecionar o projeto Python
- Executar `python <skill-dir>/scripts/inspect_python_project.py --project-root <repo>`.
- Identificar layout atual, packages, entrypoints, frameworks, testes, configuracao e sinais de arquitetura ja existente.
- Procurar primeiro por `docs/ARCHITECTURE.md`, `ARCHITECTURE.md` ou arquivo equivalente. Se existir, tratar como fonte primaria.

2. Definir ou atualizar o contrato arquitetural
- Se nao existir contrato, criar `docs/ARCHITECTURE.md` e usar `references/python-architecture-standard.md` como baseline.
- Se ja existir contrato, atualiza-lo apenas quando houver necessidade real e sempre preservando decisoes validas do projeto.
- O contrato deve registrar no minimo:
  - objetivo arquitetural
  - layout de pastas
  - limites entre modulos
  - regras de dependencia
  - onde cada tipo de codigo deve ficar
  - estrategia de testes
  - entrypoints e integracoes externas
  - como propor excecoes ou evolucoes

3. Escolher a arquitetura Python apropriada
- Preferir simplicidade e coerencia com o tamanho do projeto.
- Para servicos e APIs Python, preferir `src/` layout e separacao por responsabilidade.
- Para MVPs e backends pequenos, priorizar um pacote unico com modulos diretos por responsabilidade, como:
  - `main.py`, `dependencies.py`, `routes.py` e `schemas.py` para HTTP
  - `service.py` para regra de negocio
  - `repository.py` para persistencia
  - `crypto.py`, `qr_reader.py` e `totp.py` para integracoes concretas
  - `config.py` para configuracao
- Manter frameworks e IO nas bordas. Evitar vazar detalhes de FastAPI, Flask, banco ou CLI para o nucleo de negocio sem necessidade.
- Nao impor `domain`, `application`, `infrastructure` e `interfaces` em projetos pequenos sem justificativa real.

4. Aplicar a arquitetura em toda mudanca futura
- Antes de editar qualquer arquivo, mapear a solicitacao para o modulo, camada ou pacote correto segundo o contrato.
- Se a mudanca conflitar com o contrato, pausar e explicar o conflito.
- So introduzir nova pasta, modulo raiz ou padrao estrutural quando isso estiver alinhado ao contrato ou quando o contrato for atualizado conscientemente.
- Nao contornar o contrato por conveniencia.

5. Revisar conformidade arquitetural
- Em implementacoes ou reviews, verificar:
  - local correto do codigo
  - direcao correta das dependencias
  - ausencia de acoplamento desnecessario
  - consistencia entre codigo, testes e documentacao
- Se encontrar violacao, apontar claramente e sugerir o ajuste mais simples.

## Regras de governanca

- Ler o contrato arquitetural antes de trabalho estrutural ou refactor.
- Tratar `docs/ARCHITECTURE.md` como fonte de verdade local.
- Se faltar informacao, declarar premissas em vez de inventar.
- Se for necessario quebrar a arquitetura, pedir aprovacao e atualizar o contrato junto com a mudanca.
- Preferir evolucao incremental a reestruturacao total.

## Baseline recomendado

- `src/` para codigo de aplicacao
- `tests/` espelhando a estrutura relevante quando fizer sentido
- `docs/ARCHITECTURE.md` para o contrato
- monolito modular simples como baseline
- camadas explicitas so quando o projeto realmente justificar

## Recursos

### `scripts/inspect_python_project.py`

Inspecionar o repositorio para descobrir layout Python, packages, frameworks, testes e possiveis contratos arquiteturais.

### `references/python-architecture-standard.md`

Consultar o baseline arquitetural, a direcao de dependencias e o template do contrato `docs/ARCHITECTURE.md`.
