#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/macos"
DIST_ROOT="${ROOT_DIR}/dist/macos"
PACKAGE_DIR="${DIST_ROOT}/DragonRage-macos"
ARCHIVE_PATH="${DIST_ROOT}/DragonRage-macos.tar.gz"

mkdir -p "${DIST_ROOT}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release

rm -rf "${PACKAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${PACKAGE_DIR}"

rm -f "${ARCHIVE_PATH}"
tar -czf "${ARCHIVE_PATH}" -C "${DIST_ROOT}" "$(basename "${PACKAGE_DIR}")"

printf "macOS package created at %s\n" "${PACKAGE_DIR}"
printf "macOS archive created at %s\n" "${ARCHIVE_PATH}"
