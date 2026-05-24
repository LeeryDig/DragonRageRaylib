# Pipeline de Fases (Level Loader)

O `.glb` exportado do Blender é a fonte única da fase: visual, colliders, spawn, checkpoints e finish line. Ver [blender-track-authoring.md](blender-track-authoring.md) para convenções de criação no Blender.

## Arquivos

```
src/level/levelData.hpp
src/level/levelLoader.hpp
src/level/levelLoader.cpp
```

## Estruturas

```cpp
struct LevelCollider {
    std::string name;
    Vector3 position;
    Quaternion rotation;
    Vector3 size;
};

struct LevelCheckpoint {
    std::string name;
    int index;
    Vector3 position;
    Quaternion rotation;
    Vector3 size;
};

struct LevelSpawn {
    Vector3 position;
    Quaternion rotation;
    bool valid;
};

struct LevelData {
    std::string name;
    std::string path;
    Model visualModel;
    std::vector<LevelCollider> colliders;
    std::vector<LevelCheckpoint> checkpoints;
    LevelSpawn playerSpawn;
    LevelCheckpoint finishLine;
    bool hasFinishLine;
};
```

## API

```cpp
LevelData LoadLevel(const std::string& levelPath);
void UnloadLevel(LevelData& level);
void DrawLevel(const LevelData& level);
```

## Fluxo de carregamento

1. `LoadModel(path)` via Raylib — carrega visual para `LevelData.visualModel`.
2. `ReadGlbChunks()` — lê binário GLB (magic `0x46546C67`), extrai JSON chunk e binary chunk.
3. `ParseLevelMetadata()` — percorre nodes do glTF JSON, interpreta por nome, popula colliders/spawn/checkpoints.

## Convenção de nomes de nodes (GLB)

| Prefixo/nome | Interpretação |
|---|---|
| `VISUAL_` | visual, renderizado pelo model |
| `COL_BOX_*`, `COL_WALL_*`, `COL_OBSTACLE_*` | box collider (OBB) |
| `COL_ROAD_*`, `COL_RAMP_*`, `COL_SURFACE_*`, `COL_MESH_*` | mesh collider por triângulos |
| `SPAWN_PLAYER` | posição/rotação inicial |
| `TRIGGER_*` | volume de overlap sem colisão física |
| `CHECKPOINT_###` | checkpoint numerado (ordem pela sequência numérica) |
| `FINISH_LINE` | volume de chegada |

## Seleção de levels: `resources/config/levels.json`

```json
{
  "levels": [
    {
      "name": "Encontro de carro",
      "path": "resources/levels/car_meet.glb",
      "config": "resources/levels/car_meet.json"
    }
  ]
}
```

`path` → `Utils::ResolveProjectPath()` → `LoadModel()`.

Config runtime separada (fog, lighting, characters) carregada via `LoadLevelRuntimeConfig()` usando `config`.

## Dois tipos de collider

**Box (OBB):** usa posição, rotação e escala world do node. Cubo escalado vira box proporcional, objeto rotacionado mantém ângulo.

**Mesh/surface:** usa triângulos reais dos vértices exportados. Loader transforma para world space. Raycast da suspensão usa `GetRayCollisionTriangle()` por triângulo. Ver [road-surface-physics.md](road-surface-physics.md).
