# Python Architecture Standard

Usar esta referencia para criar ou revisar `docs/ARCHITECTURE.md`.

## Objetivo

Definir um contrato simples e executavel para projetos Python. O foco e manter clareza estrutural, baixo acoplamento e uma regra previsivel para saber onde cada tipo de codigo deve morar.

## Baseline recomendado

- Preferir layout `src/` para codigo de aplicacao.
- Manter `tests/` separado quando ele agregar valor real.
- Para MVPs e APIs pequenas, preferir monolito modular simples.
- Deixar entrypoints nas bordas: API, CLI, jobs e scripts de inicializacao.
- Centralizar o contrato em `docs/ARCHITECTURE.md`.

## Estrutura preferida para MVP simples

1. `main.py`, `dependencies.py`, `routes.py`, `schemas.py`
- FastAPI, rotas, schemas e montagem da aplicacao.
- Sao a borda HTTP do sistema.

2. `service.py`
- Regra de negocio central.
- Nao deve depender de FastAPI.

3. `repository.py`
- Persistencia e acesso a dados.

4. `crypto.py`, `qr_reader.py`, `totp.py`
- Integracoes concretas e utilitarios do dominio.

5. `config.py`
- Paths, env vars e configuracoes de runtime.

## Regras praticas

- Colocar codigo novo no modulo mais proximo da responsabilidade real.
- Evitar helpers globais sem dono claro.
- Evitar importar detalhes de infraestrutura dentro da regra de negocio sempre que isso nao custar simplicidade demais.
- Evitar criar nova pasta raiz sem necessidade.
- Nao introduzir arquitetura em camadas completas so por padrao estetico.
- Fazer testes refletirem o risco e nao apenas a arquitetura.

## Template de `docs/ARCHITECTURE.md`

```md
# Architecture

## Objetivo
- O que esta arquitetura precisa otimizar?

## Estrutura do repositorio
- Onde fica o codigo principal?
- Onde ficam testes, docs, scripts e configuracoes?

## Modulos e responsabilidades
- `src/<pacote>/main.py`
- `src/<pacote>/dependencies.py`
- `src/<pacote>/routes.py`
- `src/<pacote>/schemas.py`
- `src/<pacote>/service.py`
- `src/<pacote>/repository.py`
- `src/<pacote>/crypto.py`
- `src/<pacote>/qr_reader.py`
- `src/<pacote>/totp.py`
- `src/<pacote>/config.py`

## Regras de dependencia
- Direcao permitida das dependencias
- Dependencias proibidas

## Entry points
- API
- CLI
- Jobs
- Scripts operacionais

## Testes
- Estrategia de testes
- Localizacao dos testes

## Convencoes de mudanca
- Onde adicionar codigo novo
- Como propor excecoes
- Quando atualizar este documento
```

## Checklist de conformidade

- O contrato arquitetural existe e esta atualizado.
- Cada modulo tem responsabilidade clara.
- Dependencias seguem a direcao definida.
- Entry points ficam nas bordas.
- O projeto nao foi sobre-arquitetado sem necessidade.
- Mudancas estruturais atualizam o contrato.
