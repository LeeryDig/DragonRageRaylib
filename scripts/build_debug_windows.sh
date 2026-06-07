#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/windows-debug"
CMAKE_EXE="${CMAKE_EXE:-cmake}"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  "${CMAKE_EXE}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/toolchains/mingw-w64-x86_64.cmake"
elif command -v cl >/dev/null 2>&1; then
  "${CMAKE_EXE}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug
else
  VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
  if [[ -x "${VSWHERE}" ]]; then
    "${CMAKE_EXE}" -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G "Visual Studio 17 2022" -A x64
  else
    echo "Erro: nenhum compilador C/C++ encontrado."
    echo "Instale Visual Studio Build Tools com 'Desktop development with C++' ou MinGW-w64."
    exit 1
  fi
fi

"${CMAKE_EXE}" --build "${BUILD_DIR}" --config Debug

echo "OK: ${BUILD_DIR}/bin/DragonRage.exe"
