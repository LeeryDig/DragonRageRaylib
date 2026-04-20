#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/windows"
DIST_ROOT="${ROOT_DIR}/dist/windows"
PACKAGE_DIR="${DIST_ROOT}/DragonRage-windows"
ARCHIVE_PATH="${DIST_ROOT}/DragonRage-windows.zip"

mkdir -p "${DIST_ROOT}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/toolchains/mingw-w64-x86_64.cmake"

cmake --build "${BUILD_DIR}" --config Release

rm -rf "${PACKAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${PACKAGE_DIR}"

rm -f "${ARCHIVE_PATH}"
(
    cd "${DIST_ROOT}"
    zip -r "$(basename "${ARCHIVE_PATH}")" "$(basename "${PACKAGE_DIR}")"
)

printf "Windows package created at %s\n" "${PACKAGE_DIR}"
printf "Windows archive created at %s\n" "${ARCHIVE_PATH}"
