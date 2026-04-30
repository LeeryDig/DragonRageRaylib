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

## Build para Windows com Docker

Este fluxo gera um executável de Windows sem precisar instalar `MinGW` ou `clang++` na sua máquina.

### Requisito

- Docker Desktop ou Docker Engine com `docker compose`

### Comando

```bash
docker compose run --rm build-windows
```

### Saída

Depois do build, o jogo fica disponível em:

```bash
dist/windows/DragonRage-windows/DragonRage.exe
```

Também é gerado um pacote zipado em:

```bash
dist/windows/DragonRage-windows.zip
```

O diretório empacotado já inclui `resources/`, então o executável mantém os caminhos relativos esperados pelo jogo.

## Build para macOS sem toolchain local

Binário nativo de macOS não é gerado por este Docker de Windows. Para isso, o repositório inclui um workflow do GitHub Actions em `.github/workflows/build-macos.yml`.

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

O projeto agora usa `CMake` como base de build para reutilizar a mesma configuração no Docker e no GitHub Actions.
