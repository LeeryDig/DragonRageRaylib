---
name: git-delivery-audit-publisher
description: Auditar alteracoes reais de um repositorio Git, registrar essa auditoria localmente em uma pasta excluida do versionamento e conduzir `git add`, `git commit` e `git push` com seguranca. Usar quando Codex precisar preparar uma entrega, revisar exatamente o que mudou antes de publicar, documentar o diff real, sugerir mensagem de commit, ou concluir a publicacao de mudancas versionadas. Sempre exigir confirmacao explicita antes de executar `git add`, `git commit` ou `git push`.
---

# Git Delivery Audit Publisher

Usar esta skill para transformar uma entrega Git em um fluxo auditavel e seguro. Ela existe para evitar publicacao cega: primeiro inspecionar o diff real, depois registrar a auditoria localmente e so entao, com confirmacao explicita, executar `git add`, `git commit` e `git push`.

## Fluxo obrigatorio

1. Inspecionar o estado Git atual
- Ler `git status --short`, branch atual, remote configurado e principais arquivos alterados.
- Diferenciar arquivos staged, unstaged e untracked.
- Se houver mudancas fora do escopo pedido, explicitar isso antes de qualquer `git add`.

2. Preparar a auditoria local
- Executar `python <skill-dir>/scripts/prepare_delivery_audit_workspace.py --project-root <repo> --scope "<nome-curto-da-entrega>"`.
- Usar `delivery-audits/` como pasta padrao da auditoria.
- Confirmar que `delivery-audits/` foi adicionada ao `.git/info/exclude`.
- Ler `references/audit-report-template.md` para manter o relatorio consistente.

3. Auditar o que realmente mudou
- Executar `python <skill-dir>/scripts/write_git_audit_report.py --project-root <repo> --audit-dir <audit-dir>`.
- Registrar no relatorio:
  - branch atual
  - status do working tree
  - arquivos alterados
  - diff stat staged e unstaged
  - riscos obvios, gaps e evidencias de validacao conhecidas
- Se houver testes rodados, incluir um resumo no relatorio.

4. Parar para confirmacao explicita antes de publicar
- Antes de qualquer `git add`, `git commit` ou `git push`, apresentar:
  - escopo da entrega
  - paths que serao adicionados
  - mensagem de commit proposta
  - destino do push
- So continuar se o usuario aceitar explicitamente a publicacao.

5. Publicar com seguranca
- Preferir `git add <paths>` em vez de `git add .` quando houver risco de incluir trabalho fora de escopo.
- Usar `git commit -m "<mensagem>"` de forma nao interativa.
- Usar `git push <remote> <branch>` de forma explicita.
- Nunca usar `--force`, `--amend`, `git reset --hard` ou comandos destrutivos sem pedido expresso do usuario.

6. Fechar a auditoria
- Reexecutar o relatorio de auditoria depois do commit/push, incluindo SHA, destino do push e estado final do working tree.
- Responder com:
  - resumo da entrega
  - commit gerado
  - destino do push
  - caminho da pasta de auditoria
  - riscos ou pendencias restantes

## Regras de qualidade

- Auditar sempre antes de publicar.
- Tratar `delivery-audits/` como artefato local, nunca como parte do commit.
- Se o working tree tiver mudancas suspeitas ou fora de escopo, pausar e explicitar.
- Nao presumir que todo arquivo alterado deve ser versionado.
- Tornar a mensagem de commit fiel ao diff real, nao ao plano idealizado.

## Recursos

### `scripts/prepare_delivery_audit_workspace.py`

Criar a pasta local da auditoria Git, registrar metadata inicial e garantir a exclusao em `.git/info/exclude`.

### `scripts/write_git_audit_report.py`

Ler o estado real do Git e escrever `report.md` com status, arquivos alterados, diff stat e resumo de entrega.

### `references/audit-report-template.md`

Consultar o formato recomendado do relatorio de auditoria da entrega.

### `references/git-safety-checklist.md`

Consultar as regras de seguranca e decisao antes de `git add`, `git commit` e `git push`.
