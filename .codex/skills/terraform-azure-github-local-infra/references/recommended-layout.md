# Recommended Layout

Usar este baseline quando o repositorio ainda nao tiver infraestrutura definida.

## Recomendacao padrao

- `infra/terraform/bootstrap/`
  - cria resource group e storage account para estado remoto
- `infra/terraform/modules/project_foundation/`
  - guarda o modulo inicial com recursos base do projeto
- `infra/terraform/environments/dev/`
  - consome o modulo base para o ambiente inicial
- `.github/workflows/terraform-validate.yml`
  - valida formatacao e sintaxe em pull requests e push para `main`

## Modelo de autenticacao

- Local:
  - usar `az login`
  - confirmar a subscription ativa com `az account show`
  - usar Azure CLI como credencial de desenvolvimento
- GitHub:
  - recomendar GitHub Actions com OIDC para Azure quando houver automacao de plan/apply
  - evitar segredos permanentes por padrao

## Estrutura recomendada ao usuario

Ao apresentar a proposta, resumir em linguagem curta:

1. Pasta `infra/terraform/bootstrap` para preparar backend remoto.
2. Pasta `infra/terraform/modules/project_foundation` para o modulo base.
3. Pasta `infra/terraform/environments/dev` para o primeiro ambiente.
4. Workflow `.github/workflows/terraform-validate.yml` para fmt/init/validate.
5. Desenvolvimento local autenticado por `az login`.

## Pergunta obrigatoria de confirmacao

Antes de implementar, perguntar de forma direta:

`Posso seguir com essa infraestrutura recomendada e iniciar a implementacao?`

Se a resposta nao for uma confirmacao explicita, interromper a implementacao.

## Ajustes por contexto

- Se o projeto ja tiver infraestrutura, preservar o layout existente e apenas complementar.
- Se houver Docker, considerar evolucao futura para Container Apps ou App Service, mas nao scaffoldar isso sem necessidade.
- Se o usuario pedir integracao mais profunda com GitHub, como branch protection ou repositorio gerenciado por Terraform, avisar que isso mexe em recursos vivos antes de executar.
