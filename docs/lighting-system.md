# Sistema de Iluminação

Shader diffuse-first, sem PBR. Poucas luzes dinâmicas. Ver [PS2_VISUAL_GUIDE.md](PS2_VISUAL_GUIDE.md).

## Dados por level: `resources/levels/<name>.json`

```json
{
  "lighting": {
    "ambient": [10, 10, 15],
    "lights": [
      {
        "id": "sun",
        "type": "directional",
        "enabled": true,
        "position": [0.0, 10.0, 0.0],
        "rotation": [45.0, -35.0, 0.0],
        "color": [255, 220, 170],
        "intensity": 1.0,
        "castShadows": false
      },
      {
        "id": "spot_01",
        "type": "spot",
        "enabled": true,
        "position": [0.0, 2.0, 4.0],
        "rotation": [0.0, 180.0, 0.0],
        "color": [255, 220, 170],
        "intensity": 1.0,
        "range": 10.0,
        "innerCone": 18.0,
        "outerCone": 32.0,
        "castShadows": false
      }
    ]
  }
}
```

## Estruturas

```cpp
enum class LightType { Directional, Point, Spot };

struct LevelLightConfig {
    std::string id;
    LightType type;
    bool enabled;
    Vector3 position;
    Quaternion rotation;
    Color color;
    float intensity;
    bool castShadows;
    float range;
    float innerConeDegrees;
    float outerConeDegrees;
};

struct LightingConfig {
    Color ambient;
    std::vector<LevelLightConfig> lights;
};
```

Directional usa `rotation` para direção. Forward local `{0, 0, -1}` rotacionado = direção da luz:
```cpp
Vector3 dir = Vector3Normalize(Vector3RotateByQuaternion({0,0,-1}, light.rotation));
```

## Módulo: `src/render/lightingSystem.hpp/cpp`

```cpp
bool LoadLightingSystem(LightingSystem& lighting);
void UnloadLightingSystem(LightingSystem& lighting);
void ApplyLightingToModel(Model& model, const LightingSystem& lighting);
void UpdateLightingSystem(const LightingSystem& lighting, const LightingConfig& config, const Camera& camera);
Shader GetLightingShader(const LightingSystem& lighting);
```

Lê dados de `LevelRuntimeConfig`, não carrega JSON diretamente.

## Shader: `resources/assets/shaders/fog_lit.vs/fs`

Limites:
```glsl
#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 2
```

Uniforms por array:
- directionals: `direction`, `color`, `intensity`, `enabled`
- points: `position`, `color`, `intensity`, `range`, `enabled`
- spots: `position`, `direction`, `color`, `intensity`, `range`, `innerCone`, `outerCone`, `enabled`
- `ambientColor`, `cameraPosition`
- fog: `fogColor`, `fogStart`, `fogEnd`, `fogDensity`, `fogMode`

Vertex shader envia `fragWorldPosition` e `fragWorldNormal`.

## Shadow map (directional)

Usa primeira directional com `enabled && castShadows == true`.

**Pass 1:** cena renderizada da câmera da luz para `RenderTexture2D shadowMap` via depth shader. Guarda `lightSpaceMatrix`.

**Pass 2:** shader lit recebe `shadowMap`, `lightSpaceMatrix`, `shadowBias`, `shadowStrength`, `shadowsEnabled`. PCF simples nas bordas.

Config por level:
```cpp
bool shadowsEnabled;
int shadowResolution;   // default 1024
float shadowBias;
float shadowStrength;
float shadowAreaSize;
```

## Inspector / Debug editor

**IDs de seleção:**
```
-2              level root
<= -1000        character root (por índice)
<= -200000      character part
<= -300000      light (por índice)
```

**Lista no inspector:**
```
LIGHT sun [Directional]
LIGHT spot_01 [Spot]
```

Campos editáveis: `Enabled`, `Position` XYZ, `Rotation` XYZ, `Color` RGB, `Intensity`, `Cast Shadows`.

Botões: `Move` gizmo, `Rotate` gizmo, `Teleport to Camera`, `Save level config`.

Mudanças marcam `levelConfigDirty = true`.

## Debug draw (quando inspector aberto)

| Tipo | Representação |
|---|---|
| Directional | billboard `wb_sunny`, seta na direção da luz |
| Point | billboard `lightbulb`, esfera wire de `range` |
| Spot | billboard `flashlight_on`, cone wire de `outerConeDegrees` × `range` |

Cor = `light.color`. Selecionado = amarelo + escala maior.

## Ícones: `src/debug/debugIcons.hpp/cpp`

Fonte: `resources/icons/Material_Symbols_Rounded/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf`

```cpp
struct DebugIcons { Font materialSymbols; bool loaded; };

bool LoadDebugIcons(DebugIcons& icons);
void UnloadDebugIcons(DebugIcons& icons);
void DrawDebugIcon2D(const DebugIcons&, const char* icon, Vector2 pos, float size, Color);
void DrawDebugIconBillboard(const DebugIcons&, const Camera&, const char* icon, Vector3 worldPos, float size, Color);
```

Fallback texto se fonte não carregar: `DEL`, `SUN`, `PT`, `SP`.

| Uso | Ícone | Cor |
|---|---|---|
| Delete | `delete` | vermelho |
| Add | `add` | branco |
| Directional | `wb_sunny` | cor da luz |
| Point | `lightbulb` | cor da luz |
| Spot | `flashlight_on` | cor da luz |
| Character | `person` | branco/ciano |
| Level | `map` | branco |
| Save dirty | `save` | amarelo |

## Context menu (botão direito em debug mode)

Não abre se mouse estiver sobre painel UI.

```
Add
  Lights
    Directional Light
    Point Light
    Spot Light
```

Posição de spawn:
```cpp
spawnPos = camera.position + normalize(camera.target - camera.position) * spawnDistance;
```

Após criar: seleciona, popula inspector, marca dirty.

## Delete selected

Permite deletar `LIGHT root` e `CHAR root`. Não deleta sub-items importados do GLB.

- LIGHT: remove de `lighting.lights[index]`, limpa seleção, marca dirty, salva JSON.
- CHAR: remove de `characters[index]`, recarrega `InteractionSystem`, limpa seleção, marca dirty, salva JSON.
