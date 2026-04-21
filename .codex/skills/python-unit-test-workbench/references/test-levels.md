# Test Levels

Usar esta referencia para escolher o nivel certo de teste antes de implementar cobertura.

## Teste unitario

Usar quando:
- a regra pode ser validada no mesmo processo
- colaboracoes externas podem ser trocadas por fakes ou doubles simples
- o objetivo e proteger comportamento de modulo, servico ou funcao

Evitar quando:
- o valor real depende da UI
- a verificacao precisa de navegador
- a integracao entre varios componentes e o risco principal

## Teste de aceitacao

Usar quando:
- o criterio de aceite depende da experiencia real do usuario
- a validacao precisa de navegador, DOM, upload, navegacao ou comportamento visual
- o bug relatado so aparece ponta a ponta

Ferramenta recomendada:
- `playwright-feature-qa-auditor`

## Regra pratica

- Se a pergunta for "esta regra de negocio funciona?", comece por unitario.
- Se a pergunta for "o fluxo entregue funciona para o usuario?", escale para aceitacao.
- Se houver duvida, cubra primeiro o risco de negocio com unitario e depois valide o fluxo com Playwright.
