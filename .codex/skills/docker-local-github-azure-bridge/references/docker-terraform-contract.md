# Docker Terraform Contract

Usar este arquivo para manter a skill Docker alinhada com `terraform-azure-github-local-infra`.

## Donos de cada parte

- Skill Docker:
  - `Dockerfile`
  - `.dockerignore`
  - `docker-compose.yml`
  - workflows GitHub ligados a build, push e deploy de container
- Skill Terraform:
  - `infra/terraform/**`
  - recursos Azure
  - backend remoto, estado e modulos Terraform

## Contrato compartilhado

- Reutilizar os mesmos nomes de ambiente que a skill Terraform recomendar, como `dev` ou `homolog`.
- Reutilizar o mesmo modelo de autenticacao:
  - local com `az login`
  - GitHub com OIDC quando houver deploy em Azure
- Nao criar uma segunda arvore de infraestrutura fora de `infra/terraform/`.
- Publicar ou referenciar uma imagem container com nome previsivel.
- Fixar porta exposta e health endpoint de modo que Terraform possa usar esses valores em Container Apps, Web App for Containers ou outro alvo.

## Comportamento esperado quando a skill Terraform existir

1. Ler a skill Terraform antes de recomendar a estrutura Docker.
2. Respeitar o mesmo ritual de confirmacao antes de implementar.
3. Se Terraform recomendar GHCR, manter GHCR.
4. Se Terraform recomendar ACR, adaptar o workflow Docker para esse registry em vez de introduzir outro.
5. Se Terraform ainda nao provisionou o recurso Azure final, preparar o workflow de deploy com placeholders claros, sem fingir que a infraestrutura ja existe.

## Recomendacao padrao

- Docker local com `docker-compose.yml` na raiz.
- Pipeline GitHub para build e push da imagem.
- Workflow separado para deploy manual em Azure.
- Terraform consumindo a imagem gerada, sem assumir responsabilidade por construir a imagem.
