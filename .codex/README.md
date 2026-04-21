# Arquitetura `.codex`

Esta pasta concentra toda a infraestrutura local do Codex usada por este projeto. A ideia e manter, dentro do proprio repositorio, os agentes, skills e configuracoes que orientam o trabalho automatizado sem depender apenas da instalacao global da maquina.

## Estrutura atual

### `config.toml`

Arquivo de configuracao local do Codex para este repositorio.

Responsabilidades principais:
- habilitar recursos do Codex no projeto
- definir limites de colaboracao entre agentes
- registrar MCPs necessarios para o repositorio

Hoje ele configura:
- `multi_agent = true`
- profundidade e paralelismo dos subagents
- MCP do Playwright para execucao de QA local

## `agents/`

Contem os subagents do projeto no formato oficial do Codex: `.toml`.

Agents atuais:
- `produto.toml`: conversa sobre o produto, ajuda a explicar escopo e construir ou refinar o PRD
- `infraestrutura.toml`: cuida do levantamento DevOps, especialmente Docker, Terraform, GitHub e Azure
- `frontend.toml`: executa trabalho de interface e experiencia do usuario
- `backend.toml`: executa trabalho de API, regras de negocio, persistencia e integracoes
- `qualidade.toml`: valida entregas e executa QA com foco em evidencias
- `auditoria.toml`: compara plano versus implementacao, aponta gaps e coordena correcao

Cada agent define:
- `name`: nome pelo qual o Codex identifica o agent
- `description`: escopo resumido
- `developer_instructions`: comportamento esperado
- configuracoes opcionais como modelo, sandbox e apelidos

## `skills/`

Contem as skills locais reutilizaveis deste projeto. Cada skill possui um `SKILL.md` como ponto principal de instrucao e pode incluir pastas auxiliares.

Estrutura tipica de uma skill:
- `SKILL.md`: fluxo principal e regras da skill
- `agents/openai.yaml`: metadados de integracao
- `references/`: documentos de apoio, contratos e padroes
- `scripts/`: automacoes para checagem, scaffold ou validacao

As skills locais existem para que este repositorio possa ser testado e evoluido com autonomia, mesmo antes de mover a biblioteca para o escopo global do usuario.

## Como as pecas se relacionam

Fluxo geral:
1. `AGENTS.md` na raiz explica ao Codex como este repositorio deve ser interpretado.
2. `.codex/config.toml` aplica a configuracao local do projeto.
3. `.codex/agents/*.toml` define os papeis especializados que podem ser chamados no repositorio.
4. `.codex/skills/*/SKILL.md` fornece a base reutilizavel que esses agents seguem.

Em outras palavras:
- `AGENTS.md` explica o projeto
- `.codex/config.toml` configura o ambiente do Codex
- `.codex/agents/` organiza quem faz o trabalho
- `.codex/skills/` organiza como esse trabalho deve ser feito

## Precedencia local e global

### Configuracao do Codex

O Codex usa camadas de configuracao. Neste projeto, a regra pratica e:
1. `.codex/config.toml` do repositorio
2. `~/.codex/config.toml` do usuario

Isso significa:
- se um MCP estiver definido localmente, a configuracao local vence
- se nao estiver definido localmente, o Codex pode usar o global

### Skills

Para este projeto, a fonte canonica e local:
1. `.codex/skills/`
2. `~/.codex/skills/`

Ou seja:
- skill local tem prioridade
- skill global funciona como fallback quando necessario

## Quando editar cada parte

Edite `config.toml` quando:
- precisar adicionar ou ajustar MCPs
- quiser mudar limites de colaboracao entre agents
- quiser definir comportamento local do Codex no repositorio

Edite `agents/*.toml` quando:
- precisar mudar responsabilidades dos agents
- quiser trocar nomes, apelidos ou instrucoes de cada papel
- quiser ajustar a orquestracao entre produto, infraestrutura, frontend, backend, qualidade e auditoria

Edite `skills/*` quando:
- precisar evoluir um fluxo especializado
- quiser adicionar scripts, referencias ou contratos reutilizaveis
- quiser mover uma capacidade do projeto para uma skill global no futuro

No backend Python deste projeto, a convencao atual e:
- `python-project-architecture-guardian`: skill padrao para arquitetura simples de MVP
- `python-unit-test-workbench`: skill padrao para cobertura unitario-regressiva e decisao entre unitario e aceitacao
- layout principal em `backend-authenticator-qr-code/src/authenticator/` com `main.py`, `dependencies.py`, `routes.py`, `schemas.py`, `service.py`, `repository.py`, `crypto.py`, `qr_reader.py`, `totp.py` e `config.py`

Na trilha de DevOps e entrega deste projeto, a convencao atual tambem inclui:
- `git-delivery-audit-publisher`: skill padrao para auditar o diff real, registrar a entrega localmente e conduzir `git add`, `git commit` e `git push` com confirmacao explicita

## Objetivo desta arquitetura

Esta organizacao foi escolhida para:
- manter o projeto autocontido para testes
- separar claramente configuracao, papeis e conhecimento reutilizavel
- facilitar a futura migracao das skills do projeto para o escopo global da maquina
- deixar o comportamento do Codex mais previsivel para quem entrar no repositorio
