---
name: python-unit-test-workbench
description: Criar, atualizar e executar testes unitarios Python neste repositorio, com foco em validacao rapida de servicos, modulos e contratos locais sem depender de browser. Usar quando Codex precisar adicionar cobertura unitario-regressiva, criar scripts de execucao de testes, validar comportamento de modulos Python ou decidir entre teste unitario e teste de aceitacao. Se o pedido for teste de aceitacao, fluxo em navegador ou validacao ponta a ponta, encaminhar para a skill playwright-feature-qa-auditor.
---

# Python Unit Test Workbench

Usar esta skill para trabalhar com testes unitarios Python de forma objetiva e proporcional ao risco. Ela existe para evitar dois extremos: ausencia de testes relevantes e suites artificiais que nao validam nada importante.

## Fluxo obrigatorio

1. Classificar o nivel de teste
- Confirmar primeiro se o pedido e de teste unitario, teste de integracao leve ou teste de aceitacao.
- Se o pedido for teste de aceitacao, browser-real, fluxo E2E ou auditoria visual, usar `playwright-feature-qa-auditor` em vez desta skill.
- Se o pedido misturar unitario e aceitacao, executar primeiro o unitario e depois encaminhar a aceitacao para Playwright.

2. Inspecionar a superficie Python
- Identificar o subprojeto Python correto, entrypoints, testes existentes e comandos de execucao.
- Quando existir um runner local do projeto, preferi-lo.
- Quando nao existir, usar `python <skill-dir>/scripts/run_unittest_discovery.py --project-root <repo-ou-subprojeto>`.
- Ler `references/test-levels.md` para escolher o nivel de cobertura e evitar testes no nivel errado.

3. Implementar teste no nivel correto
- Preferir teste unitario de servico, funcao ou modulo quando a regra de negocio puder ser exercitada sem banco real, browser ou infraestrutura pesada.
- Isolar colaboracoes com doubles, fakes ou fixtures simples.
- Cobrir caminho feliz, principal borda de erro e ao menos uma regressao relevante quando houver historico de bug.
- Nao transformar teste unitario em pseudo-E2E com dependencia externa escondida.

4. Executar e validar
- Rodar apenas os testes relevantes primeiro.
- Se a suite estiver saudavel, rodar o conjunto local de testes unitarios do subprojeto.
- Registrar claramente o comando executado, o resultado e qualquer bloqueio de ambiente.

5. Encerrar com orientacao
- Responder o que foi testado, o que ficou sem cobertura e qual seria o proximo nivel de teste recomendado.
- Quando a cobertura unitario nao for suficiente para validar o comportamento pedido, recomendar explicitamente aceitacao com Playwright.

## Regras de qualidade

- Testar comportamento, nao implementacao acidental.
- Preferir poucos testes fortes a muitos testes rasos.
- Manter o teste estavel, rapido e legivel.
- Evitar mocks desnecessarios quando um fake simples resolver melhor.
- Declarar quando o pedido deveria subir para integracao ou aceitacao.

## Recursos

### `scripts/run_unittest_discovery.py`

Executar descoberta de testes `unittest` de forma generica quando o projeto nao tiver um runner proprio.

### `references/test-levels.md`

Consultar a diferenca entre teste unitario e aceitacao, incluindo o criterio de quando escalar para Playwright.

### `references/unit-test-guidelines.md`

Consultar heuristicas praticas de isolamento, checklist final e uso de `TestClient` ou doubles simples quando fizer sentido.
