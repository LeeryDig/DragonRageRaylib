#!/usr/bin/env bash

set -euo pipefail

missing=()

for command_name in cmake ninja g++ git pkg-config; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        missing+=("${command_name}")
    fi
done

if ((${#missing[@]} > 0)); then
    printf "Missing required command(s): %s\n\n" "${missing[*]}" >&2
    printf "Install the Pop!_OS/Ubuntu build dependencies with:\n\n" >&2
    printf "sudo apt-get update\n" >&2
    printf "sudo apt-get install -y build-essential cmake ninja-build git pkg-config libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev\n" >&2
    exit 1
fi
