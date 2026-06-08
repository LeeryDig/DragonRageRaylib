# Planejamento Técnico: Rádio YouTube no DragonRageRaylib

Objetivo: permitir que um rádio posicionado no mapa toque áudio vindo de um link do YouTube, usando áudio mono espacializado/simulado por distância.

---

## Estado atual já implementado

### Rádio editável no level

Já existe estrutura inicial de rádio:

- `src/audio/radioSystem.hpp`
- `src/audio/radioSystem.cpp`

O rádio é carregado pelo runtime config do level:

- `src/level/levelRuntimeConfig.hpp`
- `src/level/levelRuntimeConfig.cpp`

Config atual:

```cpp
struct LevelRadioConfig {
    std::string id;
    std::string youtubeUrl;
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    float interactionRadius;
    float audibleRadius;
    bool autoplay;
};
```

No JSON do level:

```json
"radios": [
  {
    "id": "radio_01",
    "youtubeUrl": "",
    "position": [0.0, 1.0, -3.0],
    "rotation": [0.0, 0.0, 0.0],
    "scale": [1.0, 1.0, 1.0],
    "interactionRadius": 2.0,
    "audibleRadius": 18.0,
    "autoplay": false
  }
]
```

### Inspector/editor

O rádio já aparece no Inspector como:

```txt
RADIO radio_01
```

Já é possível:

- selecionar;
- mover com gizmo;
- rotacionar;
- escalar;
- deletar;
- alternar autoplay;
- ligar/desligar visualmente;
- criar via `Shift + A -> Radio`.

Arquivos envolvidos:

- `src/editor/editorDraw.cpp`

### Interação

A regra atual do projeto é: toda interação ativa do player usa raycast da câmera.

Helper comum:

- `src/interaction/interactionRay.hpp`
- `src/interaction/interactionRay.cpp`

Funções:

```cpp
BuildInteractionRay(camera)
RayHitsInteractionBox(...)
RayHitsInteractionCapsule(...)
DrawInteractionRayDebug(...)
```

O rádio usa raycast contra box.
NPC/character usa o mesmo raycast contra cápsula.

Alcance único:

```cpp
interactionRayLength
```

Não usar `interactionDistance`.

### Debug

Quando `showForces` está ativo, o ray de interação é desenhado com:

```cpp
DrawInteractionRayDebug(camera, interactionRayLength);
```

---

## Arquitetura desejada para YouTube

Pipeline:

```txt
[Player interage com rádio]
        |
        v
[Engine C++ / Raylib]
        |
        | HTTP request /play com youtubeUrl
        v
[Servidor Node local]
        |
        | ytdl-core baixa stream
        | ffmpeg converte para PCM mono
        v
[Engine recebe PCM]
        |
        v
[AudioStream Raylib toca no rádio]
```

Formato de áudio recomendado:

```txt
PCM signed 16-bit
mono
44100 Hz
```

FFmpeg:

```bash
ffmpeg -i input -f s16le -acodec pcm_s16le -ac 1 -ar 44100 pipe:1
```

---

## Parte 1 — Servidor Node.js

Criar pasta:

```txt
tools/youtube-audio-server/
```

Arquivos:

```txt
tools/youtube-audio-server/package.json
tools/youtube-audio-server/server.js
```

Dependências:

```bash
npm init -y
npm install @distube/ytdl-core fluent-ffmpeg
```

O `ffmpeg` precisa estar instalado no PATH.

### API desejada

Servidor local em:

```txt
http://127.0.0.1:3456
```

Endpoints sugeridos:

```txt
GET /health
GET /play?url=<youtubeUrl>&port=<udpPort>
GET /stop
```

### Comportamento

`/play`:

1. recebe URL do YouTube;
2. abre stream com `@distube/ytdl-core`;
3. envia para `ffmpeg`;
4. converte para `s16le mono 44100`;
5. envia bytes PCM para a engine.

### Transporte

Duas opções:

#### Opção A — UDP

- Node envia pacotes PCM para uma porta UDP local.
- Engine escuta UDP.
- Mais simples para protótipo.
- Pode ter perda de pacotes.

#### Opção B — HTTP stream

- Engine abre conexão HTTP e lê bytes PCM direto.
- Mais confiável.
- Requer cliente HTTP streaming em C++.

Para MVP, usar UDP.

---

## Parte 2 — Cliente C++ de áudio YouTube

Criar arquivos:

```txt
src/audio/youtubeAudioClient.hpp
src/audio/youtubeAudioClient.cpp
```

Responsabilidades:

- iniciar/parar stream;
- chamar servidor Node via HTTP;
- abrir socket UDP local;
- receber bytes PCM em thread separada;
- armazenar bytes em buffer circular;
- disponibilizar amostras para Raylib `AudioStream`.

Estrutura sugerida:

```cpp
struct YouTubeAudioClient {
    bool running;
    bool connected;
    int udpPort;
    std::string currentUrl;
    AudioStream stream;
    // thread/socket/buffer circular
};
```

Funções sugeridas:

```cpp
bool InitYouTubeAudioClient(YouTubeAudioClient& client);
void ShutdownYouTubeAudioClient(YouTubeAudioClient& client);
bool StartYouTubeStream(YouTubeAudioClient& client, const std::string& url);
void StopYouTubeStream(YouTubeAudioClient& client);
void UpdateYouTubeAudioClient(YouTubeAudioClient& client);
```

`UpdateYouTubeAudioClient` deve:

- verificar se `AudioStream` precisa de dados;
- puxar PCM do buffer circular;
- preencher silêncio se faltar dado;
- chamar `UpdateAudioStream`.

---

## Parte 3 — Integrar no RadioSystem

Arquivos:

```txt
src/audio/radioSystem.hpp
src/audio/radioSystem.cpp
```

Hoje `RuntimeRadio` só tem:

```cpp
LevelRadioConfig config;
bool playing;
```

Deve passar a ter estado de áudio:

```cpp
YouTubeAudioClient audio;
float currentVolume;
```

Ou, para MVP, um único client global para o rádio ativo.

### Toggle

Alterar:

```cpp
ToggleRuntimeRadio(RuntimeRadio& radio)
```

Para:

- se desligado:
  - validar `youtubeUrl`;
  - iniciar stream;
  - `playing = true`;
- se ligado:
  - parar stream;
  - `playing = false`.

### Update

`UpdateRuntimeRadios(...)` deve:

- atualizar cliente de áudio;
- calcular volume por distância entre player e rádio;
- aplicar volume no stream.

Volume:

```cpp
float distance = Vector3Distance(playerPosition, radio.position);
float volume = 1.0f - Clamp(distance / radio.audibleRadius, 0.0f, 1.0f);
```

No futuro podemos adicionar pan estéreo fake, mas MVP é mono + volume por distância.

---

## Parte 4 — Editor/Inspector

Arquivo:

```txt
src/editor/editorDraw.cpp
```

Falta implementar campo de texto livre para editar:

```cpp
youtubeUrl
```

O input atual `DebugTextInput` aceita só números.

Criar novo helper:

```cpp
DebugStringInput(...)
```

Deve aceitar:

- letras;
- números;
- `:` `/` `?` `=` `&` `-` `_` `.` `%` etc.

No painel do rádio, adicionar:

```txt
YouTube URL: [input editável]
Interaction Ray: usa interactionRayLength global
Audible Radius: slider/campo
Autoplay ON/OFF
Radio ON/OFF
```

Observação: `interactionRadius` pode deixar de ser usado para interação ativa, porque interação agora é raycast. Podemos manter apenas se for usado para debug/futuro, mas não deve controlar foco.

---

## Parte 5 — Build/áudio Raylib

Já foi alterado:

```txt
CMakeLists.txt
```

De:

```cmake
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
```

Para:

```cmake
set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "" FORCE)
```

Quando implementar áudio real, também será necessário garantir no `main.cpp`:

```cpp
InitAudioDevice();
...
CloseAudioDevice();
```

provavelmente depois de `InitWindow()` e antes de carregar o mundo.

---

## Parte 6 — Arquivos que provavelmente serão modificados

### Novos

```txt
tools/youtube-audio-server/package.json
tools/youtube-audio-server/server.js
src/audio/youtubeAudioClient.hpp
src/audio/youtubeAudioClient.cpp
```

### Existentes

```txt
src/main.cpp
src/audio/radioSystem.hpp
src/audio/radioSystem.cpp
src/editor/editorDraw.cpp
src/gameplay/gameplayUpdate.cpp
src/gameplay/gameplayRender.cpp
src/gameplay/worldActions.cpp
src/game/gameWorld.hpp
CMakeLists.txt
```

---

## Ordem recomendada de implementação

1. Criar servidor Node com `/health` e `/play`.
2. Testar servidor isolado com um link YouTube e ffmpeg.
3. Criar `youtubeAudioClient` C++ com socket UDP e buffer circular.
4. Inicializar áudio Raylib no `main.cpp`.
5. Tocar PCM recebido em `AudioStream` sem espacialização.
6. Integrar com `RuntimeRadio`.
7. Aplicar volume por `audibleRadius`.
8. Adicionar edição real de `youtubeUrl` no Inspector.
9. Testar autoplay.
10. Só depois considerar pan estéreo ou melhorias.

---

## Regras importantes

- Interação com rádio deve continuar usando raycast unificado.
- Não usar esfera de interação no gameplay normal.
- Não usar distância pura para foco/interação.
- Distância só para volume/áudio.
- Áudio do YouTube deve ser mono para facilitar espacialização.
- O rádio deve continuar editável e salvável pelo Inspector.
