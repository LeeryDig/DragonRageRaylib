---
name: backend-service-workbench
description: Desenvolver e evoluir backends com foco em APIs, regras de negocio, persistencia, validacao, testes e prontidao de entrega. Usar quando Codex precisar implementar endpoints, casos de uso, servicos, integracoes, autenticacao, migracoes, contratos de API, documentacao tecnica ou correcoes estruturais em projetos backend, sempre respeitando a arquitetura existente e fechando com verificacao funcional.
---

# Backend Service Workbench

Usar esta skill para conduzir mudancas de backend do inicio ao fim. Ela existe para evitar implementacao superficial: primeiro inspecionar a stack e os contratos do projeto, depois alterar a camada correta, e por fim validar comportamento, testes e documentacao.

## Fluxo obrigatorio

1. Inspecionar o backend
- Executar `python <skill-dir>/scripts/inspect_backend_project.py --project-root <repo>`.
- Descobrir linguagem principal, framework, entrypoints, persistencia, testes, documentacao de API e contratos arquiteturais existentes.
- Procurar primeiro por `docs/BACKEND_ARCHITECTURE.md`, `docs/ARCHITECTURE.md` ou equivalente.
- Se o backend for Python e a skill `python-project-architecture-guardian` estiver disponivel, ler essa skill antes de fazer mudancas estruturais.
- Tratar `python-project-architecture-guardian` como a escolha padrao para MVPs e servicos simples.

2. Delimitar a mudanca
- Identificar se a solicitacao mexe em:
  - contrato HTTP ou RPC
  - regra de negocio
  - persistencia ou migracao
  - autenticacao/autorizacao
  - integracao externa
  - observabilidade ou operacao
- Ler `references/backend-delivery-standard.md` para usar o checklist adequado ao tipo de mudanca.

3. Implementar na estrutura correta
- Para MVPs, preferir um pacote Python simples com modulos diretos em `src/<pacote>/`, por exemplo `main.py`, `dependencies.py`, `routes.py`, `schemas.py`, `service.py`, `repository.py`, `crypto.py`, `qr_reader.py`, `totp.py` e `config.py`.
- Colocar parsing de entrada, transporte e serializacao nas bordas da API.
- Colocar regra de negocio em um servico central claro, sem explodir a estrutura em muitas camadas.
- Concentrar acesso a banco, QR, TOTP, criptografia e outras integracoes concretas em modulos dedicados.
- Nao misturar controller, regra de negocio e persistencia no mesmo ponto sem necessidade real.

4. Tratar contratos e dados com rigor
- Validar entrada e saida com schemas, DTOs ou validadores equivalentes ao stack.
- Definir codigos de status, mensagens de erro e comportamento em falhas.
- Se houver mudanca de dados, explicitar migracao, compatibilidade e rollback.
- Se houver autenticacao ou autorizacao, verificar caminho feliz, negacao e bordas de permissao.

5. Atualizar testes e documentacao
- Adicionar ou atualizar testes no nivel certo:
  - unidade para regra de negocio
  - integracao para repositorios, API e fluxos criticos
  - smoke ou contrato para endpoints mais importantes quando fizer sentido
- Atualizar docs de uso da API, exemplos, payloads ou referencias tecnicas quando o comportamento mudar.
- Se a mudanca alterar a arquitetura ou forma recomendada de extensao, atualizar tambem o contrato arquitetural relevante.

6. Verificar antes de encerrar
- Rodar os testes relevantes.
- Validar linter, type-check, formatacao ou comandos equivalentes quando existirem.
- Confirmar que a mudanca ficou no modulo correto, com impacto conhecido e sem quebrar o contrato do backend.
- Responder com o que mudou, o que foi validado e quais riscos ou gaps restaram.

## Regras de qualidade

- Preferir mudancas pequenas e coerentes a refactors largos sem necessidade.
- Nao introduzir endpoint sem validacao, tratamento de erro e teste razoavel.
- Nao esconder mudanca de schema ou contrato de API.
- Declarar premissas quando faltarem informacoes.
- Preservar compatibilidade quando possivel; quando nao for possivel, destacar a quebra explicitamente.

## Baseline recomendado

- contrato arquitetural do backend em `docs/BACKEND_ARCHITECTURE.md` ou `docs/ARCHITECTURE.md`
- monolito modular simples para MVPs, evitando camadas excessivas
- separacao clara entre API, regra de negocio e modulos concretos de persistencia ou integracao
- testes refletindo o nivel de risco da mudanca
- documentacao de API atualizada quando houver alteracao de comportamento

## Recursos

### `scripts/inspect_backend_project.py`

Inspecionar o repositorio para detectar stack backend, frameworks, persistencia, testes, docs e contratos arquiteturais.

### `references/backend-delivery-standard.md`

Consultar o fluxo de entrega, os checklists por tipo de mudanca e as regras de verificacao final.
