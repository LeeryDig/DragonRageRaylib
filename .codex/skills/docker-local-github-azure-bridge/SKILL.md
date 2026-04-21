---
name: docker-local-github-azure-bridge
description: Configurar Docker local de um projeto, preparar pipeline de container para GitHub e deixar a aplicacao pronta para entrega em Azure. Usar quando Codex precisar inspecionar um repositorio, recomendar Dockerfile, docker-compose, .dockerignore, workflows GitHub para imagem de container e alinhar essa entrega com a skill terraform-azure-github-local-infra antes de implementar.
---

# Docker GitHub Azure Bridge

Usar esta skill para containerizar o projeto localmente e preparar a trilha de entrega para GitHub e Azure sem conflitar com a skill de Terraform. Sempre inspecionar o repositorio primeiro, alinhar o desenho com a skill `terraform-azure-github-local-infra` quando ela existir e pedir confirmacao explicita antes de criar ou alterar arquivos.

## Fluxo obrigatorio

1. Inspecionar o repositorio e a skill Terraform
- Descobrir stack, comando de execucao, porta, healthcheck, dependencias de sistema, arquivos Docker existentes, workflows GitHub e sinais de Azure.
- Executar `python <skill-dir>/scripts/preflight_check.py --project-root <repo>`.
- Se `../terraform-azure-github-local-infra/SKILL.md` existir, ler essa skill antes de recomendar qualquer estrutura. Manter o mesmo modelo de autenticacao Azure, nomes de ambiente e postura de confirmacao.

2. Definir o contrato com Terraform
- Ler `references/docker-terraform-contract.md`.
- Tratar esta skill como dona de:
  - `Dockerfile`
  - `docker-compose.yml`
  - `.dockerignore`
  - workflows GitHub de build, validacao e deploy de container
- Tratar a skill Terraform como dona de:
  - `infra/terraform/**`
  - recursos Azure
  - backend e estado Terraform
- Compartilhar no minimo:
  - nome do ambiente
  - porta exposta pelo container
  - health endpoint
  - referencia da imagem
  - estrategia de autenticacao Azure e GitHub

3. Recomendar a arquitetura Docker
- Propor uma estrutura curta e objetiva antes de editar.
- Incluir no minimo:
  - imagem base e comando de start
  - compose local
  - estrategia de variaveis de ambiente
  - pipeline GitHub para build da imagem
  - caminho de deploy em Azure e como ele conversa com a skill Terraform
- Se houver mais de um caminho valido, preferir o menor conjunto util e citar os tradeoffs.

4. Pausar e pedir confirmacao
- Sempre parar antes de implementar.
- Fazer uma pergunta direta, por exemplo: `Posso seguir com essa configuracao Docker recomendada e iniciar a implementacao?`
- Se o usuario nao confirmar explicitamente, nao criar arquivos nem rodar comandos com efeito colateral.

5. Implementar apos confirmacao
- Criar ou atualizar os artefatos Docker de forma incremental.
- Usar `python <skill-dir>/scripts/scaffold_docker_delivery.py --project-root <repo> --project-name "<nome>"` como ponto de partida quando nao houver base Docker.
- Ajustar o scaffold ao projeto real. Nao deixar placeholders sem justificar.
- Se a skill Terraform existir, manter a integracao preparada para o layout `infra/terraform/` e para autenticao Azure por CLI local e OIDC no GitHub.

6. Validar localmente
- Rodar `docker compose config` quando `docker` estiver disponivel.
- Rodar `docker build` apenas se o custo for razoavel para o tamanho do projeto e o usuario nao tiver limitado execucao pesada.
- Validar a sintaxe dos workflows GitHub e checar paths, nomes e variaveis.
- Se houver integracao Azure pronta, validar apenas pre-requisitos e placeholders quando nao houver credenciais.

7. Encerrar com proximos passos
- Resumir o que foi recomendado, o que foi implementado e o que depende do usuario.
- Apontar os arquivos principais.
- Destacar qualquer desalinhamento pendente com Terraform, GitHub ou Azure.

## Regras de decisao

- Nao implementar sem confirmacao explicita do usuario.
- Nao duplicar responsabilidades da skill Terraform.
- Preferir compose local simples a orquestracao desnecessaria.
- Preferir GitHub Actions com OIDC para Azure quando houver deploy remoto.
- Se a skill Terraform recomendar ACR em vez de GHCR, adaptar os workflows para o mesmo registry.

## Estrutura recomendada por padrao

- `Dockerfile`
- `.dockerignore`
- `docker-compose.yml`
- `.github/workflows/container-image.yml`
- `.github/workflows/azure-container-deploy.yml`

## Recursos

### `scripts/preflight_check.py`

Verificar Docker, Docker Compose, Azure CLI, GitHub CLI, sinais de Terraform e arquivos Docker ja existentes.

### `scripts/scaffold_docker_delivery.py`

Gerar um baseline de Docker local, workflow de imagem no GitHub e workflow de deploy em Azure pronto para ser ajustado ao projeto.

### `references/docker-terraform-contract.md`

Consultar o contrato de integracao entre esta skill e `terraform-azure-github-local-infra`.
