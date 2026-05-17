# DragonRageRaylib

Protótipo de jogo de corrida 3D em C++ com raylib.

## Rodar no Pop!_OS / Ubuntu

### Dependências

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git pkg-config libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```

### Rodar localmente

```bash
bash scripts/run_linux.sh
```

Na primeira execução, o CMake baixa e compila a `raylib` 5.5 em `build/linux-debug/`.

### Gerar pacote Linux

```bash
bash scripts/build_linux.sh
```

### Saída

Depois do build, o jogo fica disponível em:

```bash
dist/linux/DragonRage-linux/DragonRage
```

Também é gerado um pacote compactado em:

```bash
dist/linux/DragonRage-linux.tar.gz
```

O diretório empacotado já inclui `resources/`, então o executável mantém os caminhos relativos esperados pelo jogo.

## Build para macOS local

### Dependências

Instale Xcode Command Line Tools e CMake:

```bash
xcode-select --install
brew install cmake ninja
```

### Build rápido local

```bash
cmake -S . -B build
cmake --build build
```

Binário gerado em:

```bash
build/bin/DragonRage
```

### Gerar pacote macOS

```bash
bash scripts/build_macos.sh
```

Saída:

```bash
dist/macos/DragonRage-macos/DragonRage
dist/macos/DragonRage-macos.tar.gz
```

O pacote inclui `resources/`, então o executável mantém os caminhos relativos esperados pelo jogo.

## Build para macOS sem toolchain local

O repositório inclui um workflow do GitHub Actions em `.github/workflows/build-macos.yml`.

### Como usar

1. Envie o projeto para o GitHub.
2. Abra a aba `Actions`.
3. Rode o workflow `Build macOS`.
4. Baixe o artifact `DragonRage-macos`.

### Saída do artifact

O workflow publica:

- `dist/macos/DragonRage-macos/DragonRage`
- `dist/macos/DragonRage-macos.tar.gz`

## Build local com CMake

O projeto usa `CMake` como base de build para reutilizar a mesma configuração em builds locais e GitHub Actions.
