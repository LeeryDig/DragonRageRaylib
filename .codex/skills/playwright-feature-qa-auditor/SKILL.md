---
name: playwright-feature-qa-auditor
description: Executar QA de uma funcionalidade especifica usando exclusivamente o MCP do Playwright, com navegacao real no browser, coleta de evidencias, relatorio de erros e criacao de uma pasta local qa-audits/ no projeto. Usar quando Codex precisar validar um fluxo web ponta a ponta, reproduzir bugs, registrar falhas com passos de reproducao e manter os artefatos fora do versionamento via .git/info/exclude.
---

# Playwright Feature QA Auditor

Usar esta skill para validar uma funcionalidade por vez em navegador real. Preparar um workspace local de auditoria no projeto, executar cenarios com o Playwright MCP, salvar evidencias e devolver um parecer curto no chat com os achados principais.

## Fluxo obrigatorio

1. Delimitar a funcionalidade
- Descobrir qual fluxo deve ser validado, qual ambiente ou URL deve ser usado, quais credenciais ou dados de teste estao disponiveis e qual comportamento esperado define sucesso.
- Limitar a auditoria a uma funcionalidade especifica. Se o pedido estiver amplo demais, reduzir para um fluxo principal e listar os demais como fora de escopo.

2. Preparar a pasta de auditoria
- Executar `python <skill-dir>/scripts/prepare_audit_workspace.py --project-root <repo> --feature "<nome-da-feature>"`.
- Usar `qa-audits/` como pasta padrao, a menos que o usuario peca outro nome.
- Confirmar que `qa-audits/` foi adicionada ao `.git/info/exclude`. Nao usar `.gitignore` para esses artefatos, exceto se o usuario pedir explicitamente.
- Registrar todos os arquivos gerados dentro da pasta criada pelo script.

3. Montar o charter de teste
- Listar os cenarios minimos antes de abrir o navegador: happy path, validacoes de formulario, mensagens de erro, persistencia ou efeito colateral, regressao visual evidente e comportamento apos refresh ou nova navegacao quando fizer sentido.
- Ler `references/audit-report-template.md` para manter o formato do relatorio e dos achados consistente.

4. Executar apenas com Playwright MCP
- Usar exclusivamente o MCP do Playwright para navegar, clicar, preencher, esperar elementos, inspecionar UI e coletar evidencias.
- A execucao deve acontecer com o navegador visivel na tela, em modo headed, mostrando o que esta acontecendo durante o fluxo.
- Se o browser nao abrir visivelmente, se a execucao ocorrer em headless ou se nao houver como acompanhar a navegacao na tela, considerar a evidencia invalida e reportar bloqueio em vez de concluir o QA.
- Nao simular execucao. Se o MCP do Playwright nao estiver disponivel, parar e reportar o bloqueio.
- Nao substituir a validacao em browser por revisao estatica de codigo. Revisao de codigo pode ajudar a orientar a investigacao, mas a conclusao de QA deve vir da execucao real.

5. Registrar evidencias e falhas
- Salvar screenshots, exports, traces ou outros artefatos dentro de `artifacts/`.
- Atualizar `report.md` com escopo, ambiente, cenarios executados, cobertura e resultado geral.
- Atualizar `findings.md` apenas com falhas reais, reproduziveis ou claramente bloqueadas.
- Para cada falha, registrar severidade, pre-condicao, passos, esperado, observado e caminho das evidencias.

6. Encerrar a entrega
- Responder no chat com um resumo curto: status geral, quantidade de falhas, bloqueios e caminho da pasta de auditoria.
- Apontar o arquivo de relatorio e o arquivo de achados.
- Sinalizar o que nao foi testado.

## Regras de qualidade

- Preferir uma execucao profunda de uma feature a uma varredura superficial de varias.
- Reproduzir cada defeito pelo menos uma segunda vez quando o custo for baixo.
- Separar bug real, bloqueio de ambiente e duvida de especificacao.
- Declarar qualquer inferencia como inferencia.
- Manter linguagem objetiva e acionavel.

## Estrutura esperada da auditoria

- `qa-audits/<timestamp>-<feature>/report.md`
- `qa-audits/<timestamp>-<feature>/findings.md`
- `qa-audits/<timestamp>-<feature>/metadata.json`
- `qa-audits/<timestamp>-<feature>/artifacts/`

## Recursos

### `scripts/prepare_audit_workspace.py`

Criar a pasta base da auditoria, abrir uma execucao nova e adicionar `qa-audits/` ao `.git/info/exclude` quando o projeto for um repositorio Git.

### `references/audit-report-template.md`

Consultar o formato do relatorio, o checklist de cobertura e o modelo dos achados antes de consolidar a entrega.
