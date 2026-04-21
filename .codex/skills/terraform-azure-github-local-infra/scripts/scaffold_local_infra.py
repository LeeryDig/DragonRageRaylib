#!/usr/bin/env python3

import argparse
import re
from pathlib import Path


def write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def render(content: str, project_name: str, prefix: str, environment: str, location: str) -> str:
    return (
        content.replace("__PROJECT_NAME__", project_name)
        .replace("__PREFIX__", prefix)
        .replace("__ENVIRONMENT__", environment)
        .replace("__LOCATION__", location)
    )


def normalize_token(value: str, fallback: str) -> str:
    normalized = re.sub(r"[^a-z0-9]+", "-", value.strip().lower()).strip("-")
    return normalized or fallback


def bootstrap_versions_tf() -> str:
    return """terraform {
  required_version = ">= 1.6.0"

  required_providers {
    azurerm = {
      source  = "hashicorp/azurerm"
      version = "~> 4.0"
    }
  }
}
"""


def bootstrap_providers_tf() -> str:
    return """provider "azurerm" {
  features {}
  subscription_id = var.subscription_id
}
"""


def bootstrap_variables_tf() -> str:
    return """variable "subscription_id" {
  description = "Azure subscription id used by the azurerm provider."
  type        = string
  default     = "00000000-0000-0000-0000-000000000000"
}

variable "project_name" {
  description = "Project name used in tags and resource names."
  type        = string
  default     = "sample-project"
}

variable "prefix" {
  description = "Lowercase prefix for Azure resource names."
  type        = string
  default     = "sample"
}

variable "environment" {
  description = "Environment name."
  type        = string
  default     = "dev"
}

variable "location" {
  description = "Azure region."
  type        = string
  default     = "eastus"
}

variable "tags" {
  description = "Additional tags."
  type        = map(string)
  default     = {}
}
"""


def bootstrap_main_tf() -> str:
    return """locals {
  storage_account_name = substr("${var.prefix}${var.environment}tfstate", 0, 24)
  tags = merge({
    project     = var.project_name
    environment = var.environment
    managed_by  = "terraform"
  }, var.tags)
}

resource "azurerm_resource_group" "tfstate" {
  name     = "${var.prefix}-${var.environment}-tfstate-rg"
  location = var.location
  tags     = local.tags
}

resource "azurerm_storage_account" "tfstate" {
  name                     = local.storage_account_name
  resource_group_name      = azurerm_resource_group.tfstate.name
  location                 = azurerm_resource_group.tfstate.location
  account_tier             = "Standard"
  account_replication_type = "LRS"
  min_tls_version          = "TLS1_2"
  tags                     = local.tags
}

resource "azurerm_storage_container" "tfstate" {
  name                  = "tfstate"
  storage_account_id    = azurerm_storage_account.tfstate.id
  container_access_type = "private"
}
"""


def bootstrap_outputs_tf() -> str:
    return """output "resource_group_name" {
  value = azurerm_resource_group.tfstate.name
}

output "storage_account_name" {
  value = azurerm_storage_account.tfstate.name
}

output "container_name" {
  value = azurerm_storage_container.tfstate.name
}
"""


def bootstrap_tfvars_example() -> str:
    return """subscription_id = "00000000-0000-0000-0000-000000000000"
project_name    = "__PROJECT_NAME__"
prefix          = "__PREFIX__"
environment     = "__ENVIRONMENT__"
location        = "__LOCATION__"

tags = {
  owner = "team-platform"
}
"""


def foundation_variables_tf() -> str:
    return """variable "project_name" {
  type = string
}

variable "prefix" {
  type = string
}

variable "environment" {
  type = string
}

variable "location" {
  type = string
}

variable "tags" {
  type    = map(string)
  default = {}
}
"""


def foundation_main_tf() -> str:
    return """locals {
  tags = merge({
    project     = var.project_name
    environment = var.environment
    managed_by  = "terraform"
  }, var.tags)
}

resource "azurerm_resource_group" "project" {
  name     = "${var.prefix}-${var.environment}-rg"
  location = var.location
  tags     = local.tags
}
"""


def foundation_outputs_tf() -> str:
    return """output "resource_group_name" {
  value = azurerm_resource_group.project.name
}

output "resource_group_location" {
  value = azurerm_resource_group.project.location
}
"""


def env_versions_tf() -> str:
    return bootstrap_versions_tf()


def env_providers_tf() -> str:
    return bootstrap_providers_tf()


def env_variables_tf() -> str:
    return bootstrap_variables_tf()


def env_main_tf() -> str:
    return """module "project_foundation" {
  source = "../../modules/project_foundation"

  project_name = var.project_name
  prefix       = var.prefix
  environment  = var.environment
  location     = var.location
  tags         = var.tags
}
"""


def env_outputs_tf() -> str:
    return """output "resource_group_name" {
  value = module.project_foundation.resource_group_name
}
"""


def env_tfvars_example() -> str:
    return bootstrap_tfvars_example()


def terraform_workflow() -> str:
    return """name: terraform-validate

on:
  pull_request:
    paths:
      - "infra/terraform/**"
      - ".github/workflows/terraform-validate.yml"
  push:
    branches:
      - main
    paths:
      - "infra/terraform/**"
      - ".github/workflows/terraform-validate.yml"

jobs:
  validate:
    runs-on: ubuntu-latest
    env:
      TF_IN_AUTOMATION: "true"
      TF_VAR_subscription_id: "00000000-0000-0000-0000-000000000000"
      TF_VAR_project_name: "__PROJECT_NAME__"
      TF_VAR_prefix: "__PREFIX__"
      TF_VAR_environment: "__ENVIRONMENT__"
      TF_VAR_location: "__LOCATION__"
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup Terraform
        uses: hashicorp/setup-terraform@v3

      - name: Terraform fmt
        run: terraform fmt -check -recursive infra/terraform

      - name: Validate bootstrap
        run: |
          terraform -chdir=infra/terraform/bootstrap init -backend=false
          terraform -chdir=infra/terraform/bootstrap validate

      - name: Validate dev environment
        run: |
          terraform -chdir=infra/terraform/environments/__ENVIRONMENT__ init -backend=false
          terraform -chdir=infra/terraform/environments/__ENVIRONMENT__ validate
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Scaffold Terraform Azure + GitHub layout.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    parser.add_argument("--project-name", required=True, help="Project name")
    parser.add_argument("--prefix", default="sample", help="Resource prefix")
    parser.add_argument("--environment", default="dev", help="Environment name")
    parser.add_argument("--location", default="eastus", help="Azure region")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    safe_prefix = normalize_token(args.prefix, "sample").replace("-", "")
    safe_environment = normalize_token(args.environment, "dev")
    safe_location = args.location.strip() or "eastus"
    environment_dir = f"infra/terraform/environments/{safe_environment}"

    files = {
        project_root / "infra/terraform/bootstrap/versions.tf": bootstrap_versions_tf(),
        project_root / "infra/terraform/bootstrap/providers.tf": bootstrap_providers_tf(),
        project_root / "infra/terraform/bootstrap/variables.tf": bootstrap_variables_tf(),
        project_root / "infra/terraform/bootstrap/main.tf": bootstrap_main_tf(),
        project_root / "infra/terraform/bootstrap/outputs.tf": bootstrap_outputs_tf(),
        project_root / "infra/terraform/bootstrap/terraform.tfvars.example": render(
            bootstrap_tfvars_example(),
            args.project_name,
            safe_prefix,
            safe_environment,
            safe_location,
        ),
        project_root / "infra/terraform/modules/project_foundation/variables.tf": foundation_variables_tf(),
        project_root / "infra/terraform/modules/project_foundation/main.tf": foundation_main_tf(),
        project_root / "infra/terraform/modules/project_foundation/outputs.tf": foundation_outputs_tf(),
        project_root / f"{environment_dir}/versions.tf": env_versions_tf(),
        project_root / f"{environment_dir}/providers.tf": env_providers_tf(),
        project_root / f"{environment_dir}/variables.tf": env_variables_tf(),
        project_root / f"{environment_dir}/main.tf": env_main_tf(),
        project_root / f"{environment_dir}/outputs.tf": env_outputs_tf(),
        project_root / f"{environment_dir}/terraform.tfvars.example": render(
            env_tfvars_example(),
            args.project_name,
            safe_prefix,
            safe_environment,
            safe_location,
        ),
        project_root / ".github/workflows/terraform-validate.yml": render(
            terraform_workflow(),
            args.project_name,
            safe_prefix,
            safe_environment,
            safe_location,
        ),
    }

    for path, content in files.items():
        write_file(path, content)

    for path in sorted(files):
        print(path)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
