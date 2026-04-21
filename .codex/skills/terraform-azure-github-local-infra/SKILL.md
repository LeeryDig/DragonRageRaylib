---
name: terraform-azure-github-local-infra
description: Planejar e implementar a infraestrutura local de um projeto com Terraform, autenticacao Azure via az login e integracao com GitHub. Usar quando Codex precisar inspecionar um repositorio, recomendar uma estrutura Terraform para Azure, preparar workflows GitHub relacionados a Terraform e pausar obrigatoriamente para confirmacao explicita do usuario antes de criar ou alterar arquivos de infraestrutura.
---

# Terraform Azure GitHub Local Infra

Usar esta skill para montar a base de infraestrutura do projeto dentro do repositorio, com foco em Terraform para Azure e integracao com GitHub. Sempre inspecionar o repositorio primeiro, recomendar uma arquitetura clara e pedir confirmacao explicita antes de qualquer implementacao.

## Fluxo obrigatorio

1. Inspecionar o projeto
- Descobrir a stack da aplicacao, estrutura atual do repositorio, existencia de `infra/`, `.github/workflows/`, Dockerfiles, pipelines, variaveis de ambiente e qualquer infraestrutura ja presente.
- Executar `python <skill-dir>/scripts/preflight_check.py --project-root <repo>` para verificar ferramentas locais e sinais de autenticacao.
- Se ja existir infraestrutura Terraform, estender ou reorganizar com cuidado. Nao substituir em bloco.

2. Recomendar uma arquitetura objetiva
- Ler `references/recommended-layout.md` para partir do baseline padrao.
- Ajustar a recomendacao ao projeto real. O padrao deve ser pequeno, implementavel e facil de evoluir.
- Incluir no minimo:
  - layout de pastas Terraform no repositorio
  - autenticacao local com `az login`
  - estrategia de estado Terraform
  - integracao com GitHub por workflow
  - variaveis, naming e proximos passos
- Preferir GitHub Actions + OIDC para automacao remota. Usar token estatico ou segredos long-lived apenas se o usuario pedir explicitamente.

3. Pausar e pedir confirmacao
- Antes de qualquer edicao, sempre mostrar a infraestrutura recomendada em poucas linhas.
- Fazer uma pergunta direta de confirmacao, por exemplo: `Posso seguir com essa infraestrutura recomendada e iniciar a implementacao?`
- Se o usuario nao confirmar explicitamente, nao criar arquivos nem rodar comandos com efeito colateral.
- Se o usuario rejeitar parte da proposta, revisar a recomendacao e pedir nova confirmacao.

4. Implementar apos confirmacao
- Criar ou atualizar a estrutura Terraform de forma incremental.
- Usar `python <skill-dir>/scripts/scaffold_local_infra.py --project-root <repo> --project-name "<nome>"` como ponto de partida quando o repositorio ainda nao tiver base de infra.
- Ajustar os templates gerados ao contexto real do projeto em vez de deixar placeholders cegamente.
- Se a implementacao incluir alteracoes vivas em GitHub, Azure ou branch protections, sinalizar o impacto antes de executar.

5. Validar localmente
- Rodar `terraform fmt -recursive` nos arquivos criados.
- Rodar `terraform init -backend=false` e `terraform validate` nos roots Terraform criados ou alterados.
- Se `az` estiver autenticado, usar `az account show` para confirmar subscription ativa. Nao forcar `az login` interativo sem necessidade.
- Validar workflows GitHub apenas no nivel sintatico e estrutural se nao houver credenciais ou federated identity prontas.

6. Encerrar com proxima acao clara
- Resumir o que foi recomendado, o que foi implementado e o que ainda depende do usuario.
- Apontar arquivos principais editados.
- Destacar qualquer bloqueio de autenticacao Azure ou GitHub.

## Regras de decisao

- Nao partir para implementacao sem confirmacao explicita do usuario.
- Nao pressupor que o projeto precisa de todos os recursos Azure. Comecar pelo menor conjunto util.
- Nao introduzir modulo Terraform ou workflow desnecessario.
- Preferir autenticacao local por Azure CLI para desenvolvimento e OIDC para GitHub Actions.
- Se houver risco de mudar recursos reais, dizer isso antes de executar.

## Estrutura recomendada por padrao

- `infra/terraform/bootstrap/`
- `infra/terraform/modules/project_foundation/`
- `infra/terraform/environments/dev/`
- `.github/workflows/terraform-validate.yml`

## Recursos

### `scripts/preflight_check.py`

Verificar se `terraform`, `az`, `gh` e `git` estao disponiveis, se ha autenticacao local basica e se o repositorio ja possui sinais de infraestrutura.

### `scripts/scaffold_local_infra.py`

Gerar uma base inicial de Terraform para Azure e um workflow de validacao no GitHub para projetos que ainda nao possuem estrutura de infraestrutura.

### `references/recommended-layout.md`

Consultar o baseline recomendado, o modelo de confirmacao e as decisoes padrao de autenticacao e integracao.
