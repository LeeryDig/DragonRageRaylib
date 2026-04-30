#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/linux"
DIST_ROOT="${ROOT_DIR}/dist/linux"
PACKAGE_DIR="${DIST_ROOT}/DragonRage-linux"
ARCHIVE_PATH="${DIST_ROOT}/DragonRage-linux.tar.gz"

"${ROOT_DIR}/scripts/check_linux_deps.sh"

mkdir -p "${DIST_ROOT}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release

rm -rf "${PACKAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${PACKAGE_DIR}"

rm -f "${ARCHIVE_PATH}"
tar -czf "${ARCHIVE_PATH}" -C "${DIST_ROOT}" "$(basename "${PACKAGE_DIR}")"

printf "Linux package created at %s\n" "${PACKAGE_DIR}"
printf "Linux archive created at %s\n" "${ARCHIVE_PATH}"
