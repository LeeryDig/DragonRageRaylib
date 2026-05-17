SHELL := /bin/bash

ROOT_DIR := $(CURDIR)
BUILD_TYPE ?= Debug
BUILD_DIR ?= $(ROOT_DIR)/build/macos-debug
TARGET := DragonRage
EXE := $(BUILD_DIR)/bin/$(TARGET)

CMAKE_GENERATOR := $(shell command -v ninja >/dev/null 2>&1 && printf 'Ninja')
CMAKE_GENERATOR_ARG := $(if $(CMAKE_GENERATOR),-G $(CMAKE_GENERATOR),)
WATCH_DIRS := src include resources CMakeLists.txt
WATCH_EXTS := cpp hpp h c json glb png jpg jpeg txt cmake

.PHONY: help setup configure build run dev package release clean distclean open

help:
	@printf "DragonRage macOS Makefile\n\n"
	@printf "Targets:\n"
	@printf "  make setup      install macOS deps with brew\n"
	@printf "  make configure  configure CMake Debug build\n"
	@printf "  make build      build Debug binary\n"
	@printf "  make run        build and run game\n"
	@printf "  make dev        auto rebuild + rerun on file change\n"
	@printf "  make package    create Release package in dist/macos\n"
	@printf "  make release    Release build only\n"
	@printf "  make clean      remove Debug build dir\n"
	@printf "  make distclean  remove build/macos* and dist/macos\n\n"
	@printf "Vars:\n"
	@printf "  BUILD_TYPE=Debug|Release  default Debug\n"
	@printf "  BUILD_DIR=path            default build/macos-debug\n"

setup:
	@if [[ "$$(uname -s)" != "Darwin" ]]; then echo "error: setup target is macOS only" >&2; exit 1; fi
	@if ! command -v brew >/dev/null 2>&1; then echo "error: Homebrew not found: https://brew.sh" >&2; exit 1; fi
	brew install cmake ninja

configure:
	@if [[ "$$(uname -s)" != "Darwin" ]]; then echo "error: this Makefile currently targets macOS" >&2; exit 1; fi
	cmake -S "$(ROOT_DIR)" -B "$(BUILD_DIR)" $(CMAKE_GENERATOR_ARG) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build "$(BUILD_DIR)" --config $(BUILD_TYPE)

run: build
	cd "$(ROOT_DIR)" && "$(EXE)"

dev:
	@echo "dev: auto rebuild + auto restart. Ctrl+C to stop."
	@if command -v fswatch >/dev/null 2>&1; then \
		trap '[[ -n "$${pid:-}" ]] && kill $$pid 2>/dev/null || true; exit 0' INT TERM EXIT; \
		while true; do \
			pid=""; \
			if $(MAKE) build; then cd "$(ROOT_DIR)" && "$(EXE)" & pid=$$!; fi; \
			fswatch -1 $(WATCH_DIRS); \
			[[ -n "$$pid" ]] && kill $$pid 2>/dev/null || true; \
			[[ -n "$$pid" ]] && wait $$pid 2>/dev/null || true; \
		done; \
	else \
		echo "fswatch not found; using polling. Install faster watcher: brew install fswatch"; \
		trap '[[ -n "$${pid:-}" ]] && kill $$pid 2>/dev/null || true; exit 0' INT TERM EXIT; \
		last=$$(find $(WATCH_DIRS) -type f \( $(foreach ext,$(WATCH_EXTS),-name '*.$(ext)' -o) -false \) -print0 2>/dev/null | xargs -0 shasum 2>/dev/null | shasum); \
		while true; do \
			pid=""; \
			if $(MAKE) build; then cd "$(ROOT_DIR)" && "$(EXE)" & pid=$$!; fi; \
			while true; do \
				now=$$(find $(WATCH_DIRS) -type f \( $(foreach ext,$(WATCH_EXTS),-name '*.$(ext)' -o) -false \) -print0 2>/dev/null | xargs -0 shasum 2>/dev/null | shasum); \
				if [[ "$$now" != "$$last" ]]; then last="$$now"; break; fi; \
				sleep 1; \
			done; \
			[[ -n "$$pid" ]] && kill $$pid 2>/dev/null || true; \
			[[ -n "$$pid" ]] && wait $$pid 2>/dev/null || true; \
		done; \
	fi

package:
	bash "$(ROOT_DIR)/scripts/build_macos.sh"

release:
	$(MAKE) build BUILD_TYPE=Release BUILD_DIR="$(ROOT_DIR)/build/macos-release"

open: build
	open "$(BUILD_DIR)/bin"

clean:
	rm -rf "$(BUILD_DIR)"

distclean:
	rm -rf "$(ROOT_DIR)/build/macos-debug" "$(ROOT_DIR)/build/macos-release" "$(ROOT_DIR)/build/macos" "$(ROOT_DIR)/dist/macos"
