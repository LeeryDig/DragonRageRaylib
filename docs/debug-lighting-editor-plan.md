# Plano: Debug Lighting Editor, Ícones, Delete e Shadow Map

## Estado atual

Já existe:

- `lighting.ambient` por level.
- `lighting.lights[]` por level.
- `LightType`: `Directional`, `Point`, `Spot`.
- Inspector lista e edita `LIGHT sun [Directional]`.
- Gizmo move/rotate para luz.
- Debug draw básico: esfera + linha.
- Shader lit+fog com ambient + primeira directional enabled.

## Assets de ícones disponíveis

Pasta atual:

```text
resources/icons/Material_Symbols_Outlined/
resources/icons/Material_Symbols_Rounded/
resources/icons/Material_Symbols_Sharp/
```

Cada pasta contém fonte `.ttf` Material Symbols. Não há PNGs ainda.

Escolha inicial:

```text
resources/icons/Material_Symbols_Rounded/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf
```

Motivo:

- Rounded combina melhor com debug UI legível.
- Ícones ficam claros em 24/32/48 px.
- Pode tintar por cor de entidade/luz.

Fallback obrigatório:

- Se fonte não carregar ou glyph não renderizar, usar texto/formas atuais.
- Se raylib não lidar bem com ligatures/codepoints da fonte variável, gerar PNGs depois em:

```text
resources/icons/generated/
```

## Ícones escolhidos

Usar Material Symbols Rounded.

| Uso | Ícone Material Symbol sugerido | Cor/tint |
|---|---|---|
| Delete | `delete` | vermelho/laranja |
| Add | `add` | branco |
| Luz genérica | `lightbulb` | cor da luz |
| Directional light | `wb_sunny` ou `light_mode` | cor da luz / amarelo quando selected |
| Point light | `lightbulb` | cor da luz |
| Spot light | `flashlight_on` ou `highlight` | cor da luz |
| Character root | `person` | branco/ciano |
| Level root | `map` ou `public` | branco |
| Save dirty | `save` | amarelo se dirty |

Nota: durante implementação validar nomes/codepoints reais. Se fonte via raylib não suportar ligature por nome, mapear codepoints ou gerar PNG.

## Debug icon system

Criar módulo pequeno:

```text
src/debug/debugIcons.hpp
src/debug/debugIcons.cpp
```

Estrutura sugerida:

```cpp
struct DebugIcons {
    Font materialSymbols;
    bool loaded;
};
```

API:

```cpp
bool LoadDebugIcons(DebugIcons& icons);
void UnloadDebugIcons(DebugIcons& icons);
void DrawDebugIcon2D(const DebugIcons& icons, const char* iconName, Vector2 pos, float size, Color color);
void DrawDebugIconBillboard(const DebugIcons& icons, const Camera& camera, const char* iconName, Vector3 worldPos, float size, Color color);
```

Fallback:

- 2D: desenhar texto curto (`DEL`, `+`, `SUN`, `PT`, `SP`).
- 3D: desenhar esfera/linha/cone atual.

## Context menu com botão direito

Em debug mode e com mouse sobre viewport:

- Clique direito abre menu no mouse.
- Não abrir se mouse estiver sobre painel UI.
- Menu fecha ao clicar fora ou escolher item.

Menu:

```text
Add
  Lights
    Directional Light
    Point Light
    Spot Light
```

Criação:

```cpp
forward = normalize(camera.target - camera.position)
spawnPos = camera.position + forward * spawnDistance
```

`spawnDistance` deve ser config/tunável, não magic perdido no código. Pode ficar como constante nomeada inicialmente em debug editor, depois config se necessário.

Defaults:

- Directional:
  - `position = spawnPos`
  - `rotation = camera forward`
  - `color = [255, 220, 170]`
  - `intensity = 1.0`
  - `castShadows = false`
- Point:
  - `position = spawnPos`
  - `range = 8.0`
  - `color = [255, 220, 170]`
  - `intensity = 1.0`
- Spot:
  - `position = spawnPos`
  - `rotation = camera forward`
  - `range = 10.0`
  - `innerConeDegrees = 18.0`
  - `outerConeDegrees = 32.0`
  - `color = [255, 220, 170]`
  - `intensity = 1.0`

Depois de criar:

- Seleciona nova luz.
- Preenche inputs do inspector.
- Marca `levelConfigDirty = true`.
- `Save level config` grava no JSON.

## Campos novos no JSON

Adicionar em `LevelLightConfig`:

```cpp
float range;
float innerConeDegrees;
float outerConeDegrees;
```

JSON:

```json
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
```

## Ambient editor

Adicionar no inspector/config sidebar:

- `Ambient R/G/B` sliders.
- Altera visual em tempo real.
- Marca dirty.
- Save grava JSON.

## Delete selected

Botão com ícone `delete` no inspector.

Escopo permitido:

- Pode deletar `CHAR root`.
- Pode deletar `LIGHT root`.
- Futuro: `PROP root`.
- Não deletar sub-itens importados do GLB:
  - visual mesh
  - collider mesh
  - icon part
  - checkpoint/collider interno se não for entidade authorável

Comportamento para `LIGHT root`:

- Remove `currentLevelRuntimeConfig.lighting.lights[index]`.
- Limpa seleção.
- Marca dirty.
- Save grava JSON.

Comportamento para `CHAR root`:

- Remove `currentLevelRuntimeConfig.characters[index]`.
- Recarrega `InteractionSystem`.
- Limpa seleção.
- Marca dirty.
- Save grava JSON.

Confirmação:

- Inicial: botão vermelho simples.
- Melhor: modal confirm depois para evitar delete acidental.

## Debug draw de luzes no cenário

Quando debug/inspector estiver aberto:

Directional:

- Ícone billboard `wb_sunny`/`light_mode` em `position`.
- Seta/linha na direção local `-Z` rotacionada.
- Cor = `light.color`.
- Selected = amarelo + escala maior.

Point:

- Ícone billboard `lightbulb` em `position`.
- Esfera wire de `range`.
- Cor = `light.color`.

Spot:

- Ícone billboard `flashlight_on`/`highlight` em `position`.
- Linha forward.
- Cone wire usando `outerConeDegrees` e `range`.
- Cor = `light.color`.

## Shader multi-light

Expandir shader atual para limites baixos:

```glsl
#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 2
```

Uniforms por arrays:

- directionals: direction/color/intensity/enabled.
- points: position/color/intensity/range/enabled.
- spots: position/direction/color/intensity/range/innerCone/outerCone/enabled.

Visual:

- Diffuse only.
- Sem PBR.
- Banding/quantização opcional para PS2 feel.
- Poucas luzes ativas.

## Shadow map direcional

Sombras primeiro só para directional.

Regra:

- Usar primeira directional `enabled && castShadows`.
- Se nenhuma, `shadowsEnabled = 0`.

Adicionar config global de lighting por level:

```cpp
bool shadowsEnabled;
int shadowResolution;
float shadowBias;
float shadowStrength;
float shadowAreaSize;
```

Pass 1:

- Criar `RenderTexture2D shadowMap`.
- Criar depth shader.
- Renderizar level + characters do ponto de vista da luz.
- Guardar `lightSpaceMatrix`.

Pass 2:

- Shader lit recebe:
  - `shadowMap`
  - `lightSpaceMatrix`
  - `shadowBias`
  - `shadowStrength`
  - `shadowsEnabled`

Cuidados:

- PCF simples.
- Bias tunável no inspector.
- Resolution 1024 default.
- Shadow strength tunável para não ficar moderno demais.
- Desligável por config.

## Ordem de implementação

1. Criar doc + escolher ícones. *(este arquivo)*
2. Criar `DebugIcons` e carregar Material Symbols Rounded.
3. Usar ícone delete no botão, com fallback texto.
4. Usar ícones billboard para lights no cenário.
5. Adicionar campos `range/innerCone/outerCone` no config/load/save.
6. Adicionar context menu botão direito: Add > Lights.
7. Adicionar delete selected para `LIGHT root` e `CHAR root`.
8. Adicionar ambient editor.
9. Expandir shader para multi directional/point/spot.
10. Implementar shadow map directional.

## Critérios de aceite

- Clique direito em debug abre menu de add.
- Add Directional/Point/Spot cria entidade à frente da câmera.
- Luz criada aparece no inspector e cenário com ícone.
- Delete remove luz/character root e salva no JSON.
- Ambient muda cena em tempo real.
- Point e Spot iluminam quando shader multi-light entrar.
- Directional com `castShadows=true` projeta sombra simples.
