#!/usr/bin/env python3

import argparse
import json
import re
from pathlib import Path


ARCHITECTURE_DOCS = [
    "docs/frontend/FRONTEND_ARCHITECTURE.md",
    "docs/FRONTEND_ARCHITECTURE.md",
    "FRONTEND_ARCHITECTURE.md",
    "docs/ARCHITECTURE.md",
    "ARCHITECTURE.md",
]

PACKAGE_JSON = "package.json"
NODE_FILES = [".nvmrc", ".node-version"]
LOCKFILES = ["pnpm-lock.yaml", "package-lock.json", "yarn.lock", "bun.lockb"]
WORKFLOW_GLOB = ".github/workflows/*"
DOCKER_GLOBS = ["Dockerfile", "Dockerfile.*", "docker/*.Dockerfile", "docker/**/*.Dockerfile"]
COMMON_FRONTEND_DIRS = ["", "frontend-backoffice", "frontend-authenticator-qr-code", "frontend", "web", "app"]


def read_text(path: Path):
    try:
        return path.read_text(encoding="utf-8-sig")
    except OSError:
        return None


def candidate_roots(project_root: Path):
    candidates = []
    for name in COMMON_FRONTEND_DIRS:
        candidate = project_root / name if name else project_root
        if candidate.exists() and candidate.is_dir():
            candidates.append(candidate)
    return candidates


def detect_workspace_root(project_root: Path):
    for candidate in candidate_roots(project_root):
        if (candidate / PACKAGE_JSON).exists():
            return candidate

    for candidate in candidate_roots(project_root):
        if any((candidate / path).exists() for path in ("src", "public", "app", "pages")):
            return candidate

    return project_root


def detect_package_manager(workspace_root: Path):
    for filename in LOCKFILES:
        if (workspace_root / filename).exists():
            if filename == "pnpm-lock.yaml":
                return "pnpm"
            if filename == "package-lock.json":
                return "npm"
            if filename == "yarn.lock":
                return "yarn"
            if filename == "bun.lockb":
                return "bun"
    return None


def parse_package_json(workspace_root: Path):
    package_json = workspace_root / PACKAGE_JSON
    if not package_json.exists():
        return None

    try:
        return json.loads(package_json.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError):
        return None


def merged_dependencies(package_json):
    if not package_json:
        return {}
    dependencies = {}
    for section in ("dependencies", "devDependencies", "peerDependencies"):
        dependencies.update(package_json.get(section, {}))
    return dependencies


def detect_stack(project_root: Path, package_json):
    deps = merged_dependencies(package_json)
    stack = []

    if "react" in deps:
        stack.append("React")
    if "next" in deps or (project_root / "next.config.js").exists() or (project_root / "next.config.ts").exists():
        stack.append("Next.js")
    if "vite" in deps or any((project_root / name).exists() for name in ("vite.config.ts", "vite.config.js", "vite.config.mjs")):
        stack.append("Vite")
    if "typescript" in deps or (project_root / "tsconfig.json").exists():
        stack.append("TypeScript")
    if "react-router-dom" in deps:
        stack.append("React Router")
    if "vitest" in deps:
        stack.append("Vitest")
    if "jest" in deps:
        stack.append("Jest")
    if "@testing-library/react" in deps:
        stack.append("React Testing Library")

    return stack


def detect_project_type(project_root: Path, package_json):
    deps = merged_dependencies(package_json)

    if "next" in deps:
        if (project_root / "app").exists():
            return "next-app-router"
        if (project_root / "pages").exists():
            return "next-pages-router"
        return "next"

    if "react" in deps and "vite" in deps:
        return "vite-react"
    if "react" in deps:
        return "react"
    return "not-detected"


def read_node_pins(project_root: Path, package_json):
    pins = {}

    if package_json and "engines" in package_json and "node" in package_json["engines"]:
        pins["package.json engines.node"] = package_json["engines"]["node"]

    for filename in NODE_FILES:
        path = project_root / filename
        if path.exists():
            content = read_text(path)
            if content is not None:
                pins[filename] = content.strip()

    workflow_versions = []
    for path in project_root.glob(WORKFLOW_GLOB):
        content = read_text(path)
        if not content:
            continue
        matches = re.findall(r"node-version\s*:\s*[\"']?([^\n\"']+)", content)
        for match in matches:
            workflow_versions.append({"file": str(path.relative_to(project_root)), "version": match.strip()})

    docker_versions = []
    for pattern in DOCKER_GLOBS:
        for path in project_root.glob(pattern):
            content = read_text(path)
            if not content:
                continue
            matches = re.findall(r"FROM\s+node:([^\s]+)", content, flags=re.IGNORECASE)
            for match in matches:
                docker_versions.append({"file": str(path.relative_to(project_root)), "image_tag": match.strip()})

    pins["workflow_node_versions"] = workflow_versions
    pins["docker_node_images"] = docker_versions
    return pins


def detect_frontend_paths(project_root: Path):
    paths = []
    for candidate in ("src", "app", "pages", "components", "public"):
        if (project_root / candidate).exists():
            paths.append(candidate)
    return paths


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect a React project architecture.")
    parser.add_argument("--project-root", required=True, help="Target repository root")
    args = parser.parse_args()

    project_root = Path(args.project_root).expanduser().resolve()
    if not project_root.exists():
        parser.error(f"Project root does not exist: {project_root}")

    workspace_root = detect_workspace_root(project_root)
    package_json = parse_package_json(workspace_root)
    architecture_docs = [doc for doc in ARCHITECTURE_DOCS if (project_root / doc).exists()]
    stack = detect_stack(workspace_root, package_json)

    result = {
        "project_root": str(project_root),
        "workspace_root": str(workspace_root.relative_to(project_root)) if workspace_root != project_root else ".",
        "react_detected": "React" in stack or "Next.js" in stack,
        "package_json_present": package_json is not None,
        "package_manager": detect_package_manager(workspace_root),
        "project_type": detect_project_type(workspace_root, package_json),
        "stack": stack,
        "frontend_paths": detect_frontend_paths(workspace_root),
        "architecture_docs": architecture_docs,
        "recommended_contract_path": architecture_docs[0] if architecture_docs else "docs/frontend/FRONTEND_ARCHITECTURE.md",
        "node_pins": read_node_pins(workspace_root, package_json),
        "scripts": package_json.get("scripts", {}) if package_json else {},
    }

    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
