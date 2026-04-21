#!/usr/bin/env python3

import argparse
import re
from pathlib import Path


def write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def render(content: str, replacements: dict[str, str]) -> str:
    for key, value in replacements.items():
        content = content.replace(key, value)
    return content


def slugify(value: str, fallback: str) -> str:
    normalized = re.sub(r"[^a-z0-9]+", "-", value.strip().lower()).strip("-")
    return normalized or fallback


def dockerfile_template() -> str:
    return """FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

WORKDIR /app

# Include common runtime libraries used by Python APIs with image and QR tooling.
RUN apt-get update && apt-get install -y --no-install-recommends \\
    curl \\
    libgl1 \\
    libglib2.0-0 \\
    libzbar0 \\
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir --upgrade pip && pip install --no-cache-dir -r requirements.txt

COPY . .

EXPOSE __PORT__

HEALTHCHECK --interval=30s --timeout=5s --start-period=20s --retries=3 CMD \\
  python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:__PORT____HEALTH_PATH__')"

CMD ["uvicorn", "__APP_MODULE__", "--host", "0.0.0.0", "--port", "__PORT__"]
"""


def dockerignore_template() -> str:
    return """__pycache__/
*.py[cod]
*.pyo
*.pyd
.Python
.pytest_cache/
.mypy_cache/
.ruff_cache/
.venv/
venv/
build/
dist/
.git/
.gitignore
.env
.env.*
qa-audits/
infra/
"""


def compose_template(with_volume: bool) -> str:
    volume_block = ""
    named_volume_block = ""
    if with_volume:
        volume_block = """    volumes:
      - __VOLUME_NAME__:__VOLUME_MOUNT__
"""
        named_volume_block = """
volumes:
  __VOLUME_NAME__:
"""

    return f"""services:
  __SERVICE_NAME__:
    build:
      context: .
      dockerfile: Dockerfile
    image: __LOCAL_IMAGE__
    container_name: __CONTAINER_NAME__
    restart: unless-stopped
    env_file:
      - .env
    environment:
      API_HOST: "0.0.0.0"
      API_PORT: "__PORT__"
__DB_PATH_BLOCK__    ports:
      - "__PORT__:__PORT__"
    healthcheck:
      test: ["CMD", "python", "-c", "import urllib.request; urllib.request.urlopen('http://127.0.0.1:__PORT____HEALTH_PATH__')"]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 20s
{volume_block}{named_volume_block}"""


def container_image_workflow_template() -> str:
    return """name: container-image

on:
  pull_request:
    paths:
      - "Dockerfile"
      - ".dockerignore"
      - "docker-compose.yml"
      - ".github/workflows/container-image.yml"
      - "requirements.txt"
      - "src/**"
  push:
    branches:
      - main
    paths:
      - "Dockerfile"
      - ".dockerignore"
      - "docker-compose.yml"
      - ".github/workflows/container-image.yml"
      - "requirements.txt"
      - "src/**"
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      packages: write
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3

      - name: Log in to GHCR
        if: github.event_name != 'pull_request'
        uses: docker/login-action@v3
        with:
          registry: ghcr.io
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}

      - name: Extract metadata
        id: meta
        uses: docker/metadata-action@v5
        with:
          images: ghcr.io/${{ github.repository }}
          tags: |
            type=sha,prefix=sha-
            type=raw,value=latest,enable={{is_default_branch}}

      - name: Build and optionally push image
        uses: docker/build-push-action@v6
        with:
          context: .
          file: ./Dockerfile
          push: ${{ github.event_name != 'pull_request' }}
          tags: ${{ steps.meta.outputs.tags }}
          labels: ${{ steps.meta.outputs.labels }}
"""


def azure_deploy_workflow_template(target: str) -> str:
    if target == "webapp":
        return """name: azure-container-deploy

on:
  workflow_dispatch:
    inputs:
      image_tag:
        description: "Tag da imagem publicada no GHCR"
        required: true
        default: "latest"

jobs:
  deploy:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      id-token: write
      packages: read
    steps:
      - name: Azure login
        uses: azure/login@v2
        with:
          client-id: ${{ secrets.AZURE_CLIENT_ID }}
          tenant-id: ${{ secrets.AZURE_TENANT_ID }}
          subscription-id: ${{ secrets.AZURE_SUBSCRIPTION_ID }}

      - name: Deploy container to Azure Web App
        uses: azure/webapps-deploy@v3
        with:
          app-name: ${{ vars.AZURE_WEBAPP_NAME }}
          images: ghcr.io/${{ github.repository }}:${{ inputs.image_tag }}
"""

    return """name: azure-container-deploy

on:
  workflow_dispatch:
    inputs:
      image_tag:
        description: "Tag da imagem publicada no GHCR"
        required: true
        default: "latest"

jobs:
  deploy:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      id-token: write
      packages: read
    steps:
      - name: Azure login
        uses: azure/login@v2
        with:
          client-id: ${{ secrets.AZURE_CLIENT_ID }}
          tenant-id: ${{ secrets.AZURE_TENANT_ID }}
          subscription-id: ${{ secrets.AZURE_SUBSCRIPTION_ID }}

      - name: Deploy container to Azure Container Apps
        uses: azure/container-apps-deploy-action@v2
        with:
          resourceGroup: ${{ vars.AZURE_CONTAINER_APP_RESOURCE_GROUP }}
          containerAppName: ${{ vars.AZURE_CONTAINER_APP_NAME }}
          imageToDeploy: ghcr.io/${{ github.repository }}:${{ inputs.image_tag }}
"""


def main() -> int:
    parser = argparse.ArgumentParser(description="Scaffold Docker + GitHub + Azure delivery files.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    parser.add_argument("--project-name", required=True, help="Project name")
    parser.add_argument("--service-name", default="app", help="Compose service name")
    parser.add_argument("--app-module", default="src.api.main:app", help="ASGI application module")
    parser.add_argument("--port", default="8000", help="Container port")
    parser.add_argument("--health-path", default="/health", help="HTTP health path")
    parser.add_argument("--azure-target", choices=["containerapp", "webapp"], default="containerapp")
    parser.add_argument("--db-path", default="", help="Optional DB path inside the container")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    service_name = slugify(args.service_name, "app")
    project_slug = slugify(args.project_name, "project")
    local_image = f"{project_slug}:local"
    container_name = f"{project_slug}-{service_name}"

    replacements = {
        "__PORT__": str(args.port),
        "__HEALTH_PATH__": args.health_path,
        "__APP_MODULE__": args.app_module,
        "__SERVICE_NAME__": service_name,
        "__LOCAL_IMAGE__": local_image,
        "__CONTAINER_NAME__": container_name,
    }

    db_path_block = ""
    volume_name = ""
    volume_mount = ""
    with_volume = False
    if args.db_path.strip():
        db_path = args.db_path.strip()
        volume_mount = str(Path(db_path).parent).replace("\\", "/")
        if volume_mount in {".", ""}:
            volume_mount = "/data"
        volume_name = f"{project_slug}-data"
        db_path_block = f'      DB_PATH: "{db_path}"\n'
        replacements["__VOLUME_NAME__"] = volume_name
        replacements["__VOLUME_MOUNT__"] = volume_mount
        with_volume = True

    replacements["__DB_PATH_BLOCK__"] = db_path_block

    files = {
        project_root / "Dockerfile": render(dockerfile_template(), replacements),
        project_root / ".dockerignore": dockerignore_template(),
        project_root / "docker-compose.yml": render(compose_template(with_volume), replacements),
        project_root / ".github/workflows/container-image.yml": container_image_workflow_template(),
        project_root / ".github/workflows/azure-container-deploy.yml": azure_deploy_workflow_template(args.azure_target),
    }

    for path, content in files.items():
        write_file(path, content)

    for path in sorted(files):
        print(path)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
