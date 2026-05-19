# Plano: Iluminação e Sombras

## Problema atual

A cena usa pipeline padrão do raylib. `DrawModel`, `DrawMesh`, `DrawCube` e similares ficam visualmente iluminados mesmo sem luz de cena configurada. Resultado: objetos parecem `fullbright`, céu escuro não afeta iluminação, não há sombras reais.

## Objetivo

Criar sistema de render com:

1. Iluminação real por shader.
2. Cena escura quando não houver luz.
3. Luz ambiente configurável.
4. Luz direcional e point lights.
5. Sombras por shadow map em fase posterior.

## Base raylib existente

Raylib não tem sistema de luz pronto na API principal, mas tem exemplo reutilizável:

- `examples/shaders/rlights.h`
- `examples/shaders/resources/shaders/glsl330/lighting.vs`
- `examples/shaders/resources/shaders/glsl330/lighting.fs`
- `examples/shaders/shaders_basic_lighting.c`

Esses arquivos servem como ponto de partida, mas devem virar sistema próprio do projeto.

## Arquivos novos sugeridos

```text
src/render/lightingSystem.hpp
src/render/lightingSystem.cpp
resources/assets/shaders/lighting.vs
resources/assets/shaders/lighting.fs
resources/config/lighting.json
```

## Configuração

Criar `resources/config/lighting.json`:

```json
{
  "ambient": [0.04, 0.04, 0.06, 1.0],
  "directional": {
    "enabled": true,
    "position": [0.0, 10.0, 0.0],
    "target": [-0.4, -1.0, -0.3],
    "color": [255, 220, 170, 255],
    "intensity": 1.0,
    "castShadows": false
  },
  "pointLights": []
}
```

Regra: valores de tuning ficam em config, não hardcoded.

## Fase 1 — Iluminação sem sombras

### 1. Criar shader lit

Adaptar shader raylib `lighting.vs/fs` para o projeto.

Uniforms mínimos:

```glsl
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 ambient;
uniform vec3 viewPos;
uniform Light lights[MAX_LIGHTS];
```

Resultado esperado:

- Sem luz ativa: renderiza só `ambient`.
- `ambient` baixo: cena fica escura.
- Luz direcional cria faces claras/escuras.
- Point light ilumina por posição.

### 2. Criar `LightingSystem`

Responsabilidades:

- Carregar shader.
- Carregar config JSON.
- Guardar luzes.
- Atualizar `viewPos` por frame.
- Atualizar uniforms das luzes.
- Aplicar shader em modelos.
- Descarregar shader.

API inicial sugerida:

```cpp
struct LightingSystem;

LightingSystem LoadLightingSystem(const char* configPath);
void UnloadLightingSystem(LightingSystem& lighting);
void UpdateLightingSystem(LightingSystem& lighting, const Camera& camera);
void ApplyLightingToModel(const LightingSystem& lighting, Model& model);
Shader GetLightingShader(const LightingSystem& lighting);
```

### 3. Integrar no carregamento de level

Depois de `LoadModel(level.path.c_str())`:

```cpp
ApplyLightingToModel(gameWorld.lighting, level.visualModel);
```

Também aplicar nos modelos de personagens/interactables/buildings quando carregados.

### 4. Primitivos

`DrawCube` usa shader default se chamado direto. Para luz real, opções:

1. Envolver com `BeginShaderMode(lighting.shader)`.
2. Melhor: transformar cubos importantes em `Model` com `GenMeshCube` e material lit.

Curto prazo: usar `BeginShaderMode` nos blocos primitivos.
Longo prazo: criar render helpers lit.

## Fase 2 — Debug de luz

Adicionar debug visual:

- Esfera/wireframe para point lights.
- Linha/seta para directional light.
- Menu debug para ligar/desligar luz ambiente, direcional e point lights.

Atalhos opcionais:

```text
L: toggle directional light
K: toggle ambient debug high/low
```

## Fase 3 — Shadow map direcional

Implementar sombras só para luz direcional primeiro.

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
- Shadow map inicial: `2048x2048` configurável.
- Área da luz precisa cobrir área jogável visível.

## Fase 4 — Materiais e compatibilidade

Garantir que shader preserve:

- textura diffuse;
- vertex color;
- `colDiffuse`;
- alpha básico;
- modelos sem textura.

Depois adicionar, se necessário:

- normal map;
- roughness/specular;
- emissive map para objetos que devem brilhar.

## Ordem recomendada de implementação

1. Copiar/adaptar `lighting.vs/fs` para `resources/assets/shaders`.
2. Criar `LightingSystem` mínimo.
3. Criar `lighting.json`.
4. Adicionar `LightingSystem lighting` em `GameWorld`.
5. Carregar lighting em `LoadGameWorld`.
6. Aplicar shader no `level.visualModel`.
7. Atualizar uniforms no frame antes de desenhar.
8. Envolver/priorizar primitivos lit.
9. Adicionar debug visual.
10. Implementar shadow map direcional.

## Critério de aceite da Fase 1

- Ambient baixo deixa cena quase escura.
- Luz direcional cria diferença clara entre faces.
- Desligar luz direcional deixa só ambient.
- Level GLB não parece mais fullbright.
- Skybox continua sem iluminação, renderizado como fundo.

## Critério de aceite da Fase 3

- Objetos grandes projetam sombra visível no chão.
- Personagem recebe sombra no chão.
- Shadow acne controlada por bias.
- Sombras podem ser desligadas por config.
