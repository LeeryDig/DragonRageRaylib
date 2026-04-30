#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/linux-debug"

"${ROOT_DIR}/scripts/check_linux_deps.sh"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" --config Debug

cd "${ROOT_DIR}"
"${BUILD_DIR}/bin/DragonRage"
