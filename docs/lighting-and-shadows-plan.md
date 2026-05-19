# Plano: Iluminação e Sombras

## Problema atual

A cena usa pipeline padrão do raylib. `DrawModel`, `DrawMesh`, `DrawCube` e similares ficam visualmente iluminados mesmo sem luz de cena configurada. Resultado: objetos parecem `fullbright`, céu escuro não afeta iluminação, não há sombras reais.

## Objetivo

Criar sistema de render com:

1. Iluminação real por shader.
2. Cena escura quando não houver luz.
3. Luz ambiente configurável por level.
4. Luz direcional inicial editável no inspector/debug mode.
5. Estrutura pronta para point lights e spot lights.
6. Sombras por shadow map em fase posterior.

## Direção visual

Seguir `docs/PS2_VISUAL_GUIDE.md`:

- Material diffuse-first, sem PBR como base.
- Poucas luzes dinâmicas.
- Fog continua importante para look PS2-era.
- Sombras modernas devem ser opcionais/tunáveis.
- Preferir resultados estilizados, baratos e editáveis.

## Base raylib existente

Raylib não tem sistema de luz pronto na API principal, mas tem exemplo reutilizável:

- `examples/shaders/rlights.h`
- `examples/shaders/resources/shaders/glsl330/lighting.vs`
- `examples/shaders/resources/shaders/glsl330/lighting.fs`
- `examples/shaders/shaders_basic_lighting.c`

Esses arquivos servem como referência, mas devem virar sistema próprio do projeto.

## Configuração: JSON por level

A iluminação deve ficar no JSON do level, junto de skybox, fog e characters.

Exemplo:

```text
resources/levels/car_meet.glb   // geometria/visual/colliders exportados do Blender
resources/levels/car_meet.json  // skybox, fog, lights, characters, overrides editáveis
```

Não usar `resources/config/lighting.json` global agora. Luz é composição do level.

Motivos:

- Cada mapa pode ter clima/horário/luz diferente.
- Inspector já edita e salva `currentLevelRuntimeConfig`.
- Fog/skybox/characters já são por level.
- Futuro menu `Add Light` adiciona luz no level atual.
- JSON é bom como formato de authoring/editor: simples, legível, bom no git, fácil de salvar.

Futuro possível:

- Manter JSON como formato editável.
- Gerar formato binário/cooked para build final se o projeto crescer.
- Importar luzes do GLB como defaults, mas salvar overrides no JSON.

## Estrutura de dados desejada

Usar lista genérica desde o começo, mesmo com só uma directional light agora.

```cpp
enum class LightType {
    Directional,
    Point,
    Spot
};

struct LevelLightConfig {
    std::string id;
    LightType type;
    bool enabled;
    Vector3 position;
    Quaternion rotation;
    Color color;
    float intensity;
    bool castShadows;
};

struct LightingConfig {
    Color ambient;
    std::vector<LevelLightConfig> lights;
};
```

Directional usa:

- `position`: origem visual/debug no editor.
- `rotation`: direção da luz. Forward local `{0, 0, -1}` aponta para onde a luz ilumina.
- `color`: cor RGB.
- `intensity`: força.
- `castShadows`: reservado para Fase 3.

Point/spot futuros reaproveitam `position`, `rotation`, `color`, `intensity` e adicionam campos como `range`, `innerCone`, `outerCone` quando necessário.

## JSON sugerido

```json
{
  "skybox": "resources/assets/skybox/Cubemap_Sky_01-512x512.png",
  "fog": {
    "enabled": true,
    "color": [20, 22, 30],
    "start": 45.0,
    "end": 150.0,
    "density": 1.0,
    "mode": "linear"
  },
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
      }
    ]
  },
  "characters": []
}
```

Regra: valores de tuning ficam em config, não hardcoded.

## Sistema de render

### Shader único de cena

No curto prazo, preferir shader único para:

- diffuse texture;
- vertex color;
- `colDiffuse`;
- ambient;
- directional light;
- fog.

Opções de arquivo:

```text
resources/assets/shaders/scene_lit.vs
resources/assets/shaders/scene_lit.fs
```

Ou evoluir `fog_lit.vs/fs` primeiro e refatorar nome depois.

### `LightingSystem` / `SceneShader`

Criar sistema modular, mas pequeno.

Arquivos sugeridos:

```text
src/render/lightingSystem.hpp
src/render/lightingSystem.cpp
```

Responsabilidades:

- Carregar shader lit.
- Guardar uniform locations.
- Aplicar shader em modelos.
- Atualizar uniforms por frame.
- Ler dados de `LevelRuntimeConfig`, não carregar JSON diretamente.
- Descarregar shader.

API inicial sugerida:

```cpp
struct LightingSystem;

bool LoadLightingSystem(LightingSystem& lighting);
void UnloadLightingSystem(LightingSystem& lighting);
void ApplyLightingToModel(Model& model, const LightingSystem& lighting);
void UpdateLightingSystem(const LightingSystem& lighting, const LightingConfig& config, const Camera& camera);
Shader GetLightingShader(const LightingSystem& lighting);
```

Nota: como fog já existe, caminho mais limpo pode ser renomear conceitualmente `FogShader` para `SceneShader` depois. Primeiro patch pode manter nomes antigos para reduzir risco.

## Fase 1 — Directional light sem sombras

### 1. Config por level

Adicionar `lighting` em `LevelRuntimeConfig`:

- `ambient`.
- lista `lights`.
- criar default directional `sun` se JSON não tiver luz.

### 2. Shader lit + fog

Uniforms mínimos:

```glsl
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 cameraPosition;
uniform vec3 ambientColor;
uniform int directionalLightEnabled;
uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;
uniform float directionalLightIntensity;
```

Vertex shader precisa enviar:

```glsl
out vec3 fragWorldPosition;
out vec3 fragWorldNormal;
```

Resultado esperado:

- Sem luz ativa: renderiza só ambient.
- Ambient baixo: cena fica escura.
- Directional cria faces claras/escuras.
- Fog continua funcionando.
- Skybox continua sem iluminação.

### 3. Integração com modelos

Aplicar shader em:

- `level.visualModel`.
- personagens/interactables carregados.
- outros models relevantes.

Primitivos (`DrawCube`, etc.) ainda usam shader default se chamados direto. Curto prazo:

1. Envolver primitivos importantes com `BeginShaderMode` se necessário.
2. Longo prazo: criar render helpers lit ou transformar cubos em `Model` com `GenMeshCube`.

## Fase 2 — Inspector/debug mode para luz

A luz deve ser selecionável e editável como entidade do level.

### IDs de seleção

Reservar range negativo para luzes:

```cpp
LightSelectionId(index) = -300000 - index;
```

Atual existe:

- `-2`: level root.
- `<= -1000`: character root.
- `<= -200000`: character part.

### Lista no inspector

Na aba/root do inspector, mostrar:

```text
LEVEL car_meet
CHAR npc_01
LIGHT sun [Directional]
```

Ao clicar na luz:

- seleciona `currentLevelRuntimeConfig.lighting.lights[index]`.
- popula inputs de position/rotation.

### Painel de edição da luz

Campos iniciais:

- `Enabled` toggle.
- `Position` XYZ.
- `Rotation` XYZ.
- `Color` RGB sliders.
- `Intensity` slider.
- `Cast Shadows` placeholder/off por enquanto.

Botões:

- `Move` gizmo.
- `Rotate` gizmo.
- `Teleport to Camera`.
- `Save level config`.

Mudanças marcam:

```cpp
gameWorld.debugUi.levelConfigDirty = true;
```

### Gizmo

Integrar luz em:

```cpp
GetSelectedTransform(...)
SetSelectedTransform(...)
```

Para directional light:

- Move altera `light.position`.
- Rotate altera `light.rotation`.
- Scale ignorado.

Direção usada pelo shader:

```cpp
Vector3 direction = Vector3Normalize(Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, light.rotation));
```

### Debug draw

Quando debug/inspector estiver aberto:

- desenhar esfera pequena em `light.position`.
- desenhar linha/seta na direção da luz.
- cor baseada em `light.color`.
- destaque amarelo quando selecionada.

## Fase 3 — Menu futuro `Add Light`

Depois da directional inicial, adicionar menu para criar luzes no level atual.

Opções futuras:

```text
Add Directional Light
Add Point Light
Add Spot Light
```

Cada ação adiciona item em:

```cpp
gameWorld.currentLevelRuntimeConfig.lighting.lights
```

E salva no JSON do level.

Campos futuros:

- Point: `range`.
- Spot: `range`, `innerCone`, `outerCone`.
- Todos: `enabled`, `position`, `rotation`, `color`, `intensity`, `castShadows`.

## Fase 4 — Shadow map direcional

Implementar sombras só para directional light primeiro.

### Pass 1: depth from light

- Criar `RenderTexture2D shadowMap`.
- Criar shader depth-only.
- Renderizar cena da câmera da luz.
- Guardar matriz `lightSpaceMatrix`.

### Pass 2: render normal

- Enviar `shadowMap` ao shader lit.
- Enviar `lightSpaceMatrix`.
- No fragment shader:
  - transformar posição mundo para light space;
  - comparar depth atual com depth do shadow map;
  - aplicar fator de sombra.

Uniforms adicionais:

```glsl
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform float shadowBias;
uniform int shadowsEnabled;
```

### Cuidados

- Bias evita shadow acne.
- PCF simples suaviza bordas.
- Shadow map inicial configurável, talvez 1024/2048.
- Área da luz precisa cobrir área jogável visível.
- Sombras devem poder desligar por config.
- Para look PS2, considerar blob/projected shadows antes de sombras modernas complexas.

## Fase 5 — Materiais e compatibilidade

Garantir que shader preserve:

- textura diffuse;
- vertex color;
- `colDiffuse`;
- alpha básico;
- modelos sem textura.

Depois adicionar, se necessário:

- fake/specular simples;
- emissive map para objetos que devem brilhar;
- normal map apenas se combinar com direção visual.

Evitar default PBR/roughness/metalness.

## Ordem recomendada de implementação

1. Ajustar `LevelRuntimeConfig` para `lighting.ambient` + `lights[]`.
2. Fazer load/save do JSON por level.
3. Criar/evoluir shader lit + fog.
4. Criar `LightingSystem` ou evoluir `FogShader` para shader de cena.
5. Adicionar runtime em `GameWorld`.
6. Aplicar shader no `level.visualModel` e personagens.
7. Atualizar uniforms por frame antes de desenhar.
8. Adicionar seleção de luz no inspector.
9. Integrar luz no gizmo move/rotate.
10. Adicionar debug draw da directional.
11. Adicionar menu futuro `Add Light`.
12. Implementar shadow map direcional.

## Critério de aceite da Fase 1

- Ambient baixo deixa cena quase escura.
- Directional light cria diferença clara entre faces.
- Desligar directional deixa só ambient.
- Level GLB não parece mais fullbright.
- Skybox continua renderizado como fundo, sem iluminação.
- Fog continua funcionando.

## Critério de aceite da Fase 2

- Directional aparece no inspector como `LIGHT sun [Directional]`.
- Clicar seleciona luz.
- Position/rotation/color/intensity mudam visual em tempo real.
- Move/rotate gizmo funcionam na luz.
- Save grava luz no JSON do level.
- Debug draw mostra origem e direção da luz.

## Critério de aceite da Fase 4

- Objetos grandes projetam sombra visível no chão.
- Personagem recebe sombra no chão.
- Shadow acne controlada por bias.
- Sombras podem ser desligadas por config.
