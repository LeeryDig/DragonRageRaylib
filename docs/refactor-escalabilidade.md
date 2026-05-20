# Refactor de Escalabilidade — Dragon Rage

Documento de referência para as melhorias arquiteturais feitas. Cada seção explica o **problema**, **o que foi feito** e **o que fica possível agora**.

---

## Contexto do jogo

Jogo de interação com muitos NPCs — **não é um RPG**. Não há inventário, quests clássicas ou stats. O foco é escalar o número de entidades e interações sem que o código quebre. Cada decisão aqui aponta nessa direção.

---

## Fase 1 — Modularização do main.cpp

### Problema
`main.cpp` tinha lógica de gameplay, render e inicialização misturados. Qualquer mudança em render afetava o mesmo arquivo que gerenciava o loop principal.

### O que foi feito
`main.cpp` foi reduzido a um entry-point de ~55 linhas. A lógica foi movida para módulos separados:

| Arquivo | Responsabilidade |
|---|---|
| `gameplay/gameplayUpdate.cpp` | Física, input, câmera — update frame |
| `gameplay/gameplayRender.cpp` | Draw chamadas em ordem, debug overlays |
| `gameplay/worldActions.cpp` | Carregamento/reset de nível, load de configs |

### O que fica possível
- Trocar o loop de render sem tocar na lógica de gameplay.
- Adicionar modos (pause, cutscene, menu) como estados que substituem `UpdateGameplay`/`DrawGameplay`.

---

## Fase 2 — Escalabilidade de NPCs

### 2a. EntityRegistry com IDs estáveis

#### Problema
NPCs viviam em `vector<InteractableCharacter>` com acesso por índice. Adicionar ou remover um NPC invalidava todos os índices anteriores — impossível manter referências entre frames.

#### O que foi feito
`InteractionSystem` foi substituído por `EntityRegistry` (`src/entity/`):

```
EntityId  →  uint32_t, começa em 1, INVALID_ENTITY = 0
EntityRegistry  →  dois vetores paralelos: characterIds[] + characters[]
```

Operações-chave:
- `AddCharacter` / `RemoveCharacter` — gerenciam IDs automaticamente
- `FindCharacter(id)` — busca por ID, não por índice
- `focusedId` / `activeDialogueId` — referências que não quebram com remoção

O editor ainda usa índice (`CharacterAt(registry, i)`) porque precisa de ordem determinística — as duas abordagens coexistem.

#### O que fica possível
- Remover NPCs em runtime sem corromper referências de outros sistemas.
- Bases para: NPCs que morrem, que aparecem por evento, grupos de NPCs com IDs.
- Futuro: adicionar componentes a entidades via ID (posição, estado de diálogo, comportamento).

---

### 2b. GameWorld dividido em subsistemas

#### Problema
`GameWorld` era um struct gigante com todos os campos misturados. Qualquer função precisava receber `GameWorld&` inteiro mesmo que usasse só dois campos. Isso criou acoplamento global — difícil saber quem depende de quê.

#### O que foi feito
`GameWorld` foi dividido em quatro subsistemas nomeados:

```cpp
struct GameWorld {
    WorldContext  world;   // level, statics, levelsConfig, runtimeConfig
    PlayerContext player;  // config, state, input, physics, physicsAccumulator
    EntityRegistry npcs;   // todos os personagens
    RenderContext render;  // camera, fogShader, debugIcons, chaseCamera, debugCamera
    DebugUiState debugUi;  // estado do editor/debug
};
```

Acesso via `gameWorld.player.state.position`, `gameWorld.world.level`, `gameWorld.render.camera`, etc.

Código morto removido junto: `VehicleConfig`, `VehicleState`, `CarVisual`, painéis de tuning de veículo, `TeleportVehicle`.

#### O que fica possível
- Funções podem declarar dependência real: `void Foo(PlayerContext& player)` em vez de `void Foo(GameWorld& gw)`.
- Preparação para multi-threading: `WorldContext` e `PlayerContext` são candidatos a locks separados.
- Adicionar subsistemas novos (ex: `AudioContext`, `UIContext`) sem tocar nos existentes.

---

## Fase 3 — Qualidade de vida

### 3a. Camada de input abstrata (InputMap)

#### Problema
Teclas físicas hardcoded em todo o código — `IsKeyDown(KEY_W)`, `IsKeyPressed(KEY_E)`. Trocar uma tecla exigia grep em vários arquivos. Diálogo e movimento usavam keys duplicadas sem contrato.

#### O que foi feito
`src/input/inputMap.hpp/.cpp` com enum de ações e binding por JSON:

```cpp
enum class GameAction { MoveForward, MoveBack, MoveLeft, MoveRight, Interact, OpenMenu };

struct InputMap { int moveForward, moveBack, moveLeft, moveRight, interact, openMenu; };

bool IsActionDown(const InputMap&, GameAction);
bool IsActionPressed(const InputMap&, GameAction);
```

Bindings carregados de `resources/config/input.json`:
```json
{ "move_forward": "W", "move_back": "S", "interact": "E", "open_menu": "ESCAPE" }
```

`InputMap` vive em `PlayerContext` (`gameWorld.player.input`). Diálogo usa `MoveForward`/`MoveBack` para navegar e `Interact` para confirmar — mesmas teclas do gameplay, sem duplicação.

#### O que fica possível
- Remapping de teclas em runtime: recarregar o JSON e atribuir `gameWorld.player.input`.
- Suporte a gamepad: adicionar `int gamepadButton` ao `InputMap` e `IsActionDown` verifica os dois.
- Adicionar ações novas (`Sprint`, `Crouch`, `OpenMap`) sem tocar em código existente.

---

### 3b. Centralização de constantes (world.json)

#### Problema
`main.cpp` tinha `1280`, `720`, `"Dragon Rage"`, `60` hardcoded como literais e uma constante `1.0f / 60.0f` derivada implicitamente do FPS.

#### O que foi feito
`src/game/worldConfig.hpp/.cpp` carrega `resources/config/world.json` **antes** de `InitWindow`:

```json
{ "window_width": 1280, "window_height": 720, "window_title": "Dragon Rage", "target_fps": 60 }
```

`main.cpp` usa `wc.windowWidth`, `wc.targetFps` etc. A constante derivada virou `1.0f / static_cast<float>(wc.targetFps)`.

#### O que fica possível
- Builds diferentes (desktop 1920×1080, portátil 1280×720) sem recompilar — só trocar o JSON.
- Futuro: adicionar `fullscreen: true`, `vsync: true` ao mesmo arquivo.

---

## Estado atual do código

```
src/
├── main.cpp                    ← entry point ~55 linhas
├── game/
│   ├── gameWorld.hpp           ← GameWorld + 4 subsistemas
│   └── worldConfig.hpp/.cpp    ← WorldConfig + load
├── gameplay/
│   ├── gameplayUpdate.cpp      ← UpdateGameplay
│   ├── gameplayRender.cpp      ← DrawGameplay
│   └── worldActions.cpp        ← LoadGameWorld, RestartLevel, etc.
├── entity/
│   ├── entityId.hpp            ← EntityId = uint32_t
│   └── entityRegistry.hpp/.cpp ← EntityRegistry, CRUD por ID
├── input/
│   └── inputMap.hpp/.cpp       ← GameAction enum, bindings, IsActionDown/Pressed
├── personController.hpp/.cpp   ← PersonState, PersonConfig, ReadPersonInput(inputMap)
└── interactionSystem.hpp/.cpp  ← dados de NPC (InteractableCharacter, etc.)

resources/config/
├── world.json    ← janela, FPS
├── input.json    ← bindings de teclas
├── person.json   ← config de movimento do player
├── camera.json   ← config de câmera
└── levels.json   ← lista de níveis
```

---

## O que ainda precisa atenção

### Débito técnico imediato
- `interactionSystem.hpp/.cpp` ainda existe como arquivo de dados de NPC — o nome é enganoso (não é mais um "sistema"). Renomear para `character.hpp/.cpp` ou `npcData.hpp/.cpp` deixaria mais claro.
- `vehiclePhysics.cpp/.hpp` ainda compilam mas são código morto. Podem ser removidos.
- `roads.cpp`, `scenery.cpp` — verificar se são usados ou também são mortos.

### Próximos passos naturais para escalar NPCs

**Comportamento de NPC** — hoje os NPCs são estáticos. Para um jogo de interação com muitos deles, o próximo passo é um sistema de estado simples por NPC (idle, talking, walking para ponto X). Não precisa ser IA complexa — uma máquina de estados com 3-4 estados já desbloqueia muito.

**Diálogo por arquivo** — hoje o diálogo é hardcoded nos GLB/JSON de cada personagem. Separar o conteúdo de diálogo em arquivos próprios (ex: `resources/dialogue/npc_aldeao.json`) permite editar texto sem recarregar assets 3D.

**Zona de trigger** — para ativar NPCs, músicas ou eventos quando o player entra numa área. O `LevelData` já tem nós do tipo `Trigger` no debug — falta o sistema que os lê em runtime.

**Culling de NPCs** — com muitos NPCs, desenhar todos todo frame vai pesar. Um sistema simples de distância (só atualiza NPCs dentro de X metros) é suficiente por muito tempo antes de precisar de spatial hashing.
