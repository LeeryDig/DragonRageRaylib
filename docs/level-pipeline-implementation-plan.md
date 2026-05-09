# Plano de implementação da pipeline de fases/pistas

Este documento descreve o plano para implementar carregamento de fases criadas no Blender usando arquivos `.glb`.

Objetivo: permitir criar quantas fases forem necessárias sem alterar código para cada pista nova.

## Decisão de arquitetura

O arquivo `.glb` da fase será a fonte principal dos dados:

- visual da pista;
- objetos de cenário;
- colliders;
- spawn do jogador;
- checkpoints;
- finish line.

O jogo deverá carregar o mesmo `.glb` de duas formas:

1. **Raylib `LoadModel()`** para renderização visual.
2. **Parser glTF** para ler nodes, nomes, transforms e dados de gameplay.

Bibliotecas candidatas para parsing glTF:

- `cgltf` — simples, C, header-only, boa opção para começar.
- `tinygltf` — C++, mais completa, mas maior.

Recomendação inicial: **cgltf**.

## Estrutura de pastas

```txt
resources/levels/
  city_01/
    city_01.glb
  desert_01/
    desert_01.glb
```

No futuro, pode existir um índice de fases, mas o MVP pode carregar uma fase fixa por caminho.

## Novas estruturas sugeridas

Criar arquivos:

```txt
src/level/levelData.hpp
src/level/levelLoader.hpp
src/level/levelLoader.cpp
```

Estruturas iniciais:

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

## Loader de fase

API sugerida:

```cpp
LevelData LoadLevel(const std::string& levelPath);
void UnloadLevel(LevelData& level);
void DrawLevel(const LevelData& level);
```

Responsabilidades:

- carregar modelo visual com `LoadModel()`;
- abrir o `.glb` com parser glTF;
- percorrer nodes;
- interpretar nodes pelo nome;
- extrair transform global/local;
- popular colliders, spawn, checkpoints e finish line.

## Convenções de leitura

| Nome | Interpretação |
|---|---|
| `COL_` | Box collider |
| `SPAWN_PLAYER` | Spawn do jogador |
| `CHECKPOINT_###` | Checkpoint numerado |
| `FINISH_LINE` | Volume de chegada |
| `VISUAL_` | Visual, já renderizado pelo model |

## Etapas de implementação

### Etapa 1 — Preparar estrutura de LevelData

- Criar `src/level/levelData.hpp`.
- Definir estruturas de fase.
- Adicionar `UnloadLevel()` e `DrawLevel()`.
- Inicialmente, `DrawLevel()` apenas chama `DrawModel()`.

### Etapa 2 — Carregar visual da fase

- Criar `LoadLevel(path)` usando `LoadModel(path.c_str())`.
- Substituir ou adaptar `StaticWorld` para aceitar uma fase carregada.
- Fazer o jogo desenhar a fase `.glb` no lugar da estrada hardcoded.

Critério de sucesso:

- Um `.glb` exportado do Blender aparece no jogo.

### Etapa 3 — Integrar parser glTF

- Adicionar `cgltf` ao projeto, provavelmente em:

```txt
src/third_party/cgltf.h
```

- Criar função para abrir `.glb` e percorrer nodes.
- Imprimir/logar nomes dos nodes encontrados para validar exportação do Blender.

Critério de sucesso:

- O jogo consegue listar no log nomes como `COL_ROAD_001`, `SPAWN_PLAYER`, `CHECKPOINT_001`.

### Etapa 4 — Extrair transforms

Implementar extração de:

- posição;
- rotação;
- escala;
- matriz local/global se necessário.

Ponto importante: validar conversão de eixos Blender/glTF/raylib.

Critério de sucesso:

- `SPAWN_PLAYER` posiciona corretamente o carro.

### Etapa 5 — Criar colliders físicos a partir de `COL_`

Para cada node começando com `COL_`:

- calcular centro;
- calcular rotação;
- calcular tamanho;
- criar shape/collider no `PhysicsWorld`.

No MVP, todos os `COL_` serão box colliders.

Critério de sucesso:

- O carro anda sobre a pista exportada do Blender usando colliders do Blender.

### Etapa 6 — Checkpoints e finish line

Para cada `CHECKPOINT_###`:

- ler índice;
- ordenar checkpoints;
- criar volumes de trigger.

Para `FINISH_LINE`:

- criar volume de trigger final.

Critério de sucesso:

- O jogo detecta passagem por checkpoint e chegada.

### Etapa 7 — Remover dependência da pista hardcoded

Hoje `StaticWorld` possui estrada, grama e prédios hardcoded.

Depois da pipeline funcionando:

- remover/desativar geração hardcoded de estrada;
- manter somente o que for necessário como fallback/debug;
- mover lógica de pista para `LevelData`/`LevelLoader`.

### Etapa 8 — Seleção/carregamento de múltiplas fases

Adicionar algum mecanismo para escolher fase:

Opção simples inicial:

```cpp
const char* LEVEL_PATH = "resources/levels/city_01/city_01.glb";
```

Depois evoluir para:

```txt
resources/levels/levels.txt
```

ou descoberta automática de pastas.

## Pontos técnicos importantes

### Render visual vs colliders

O visual será desenhado pelo `Model` carregado com Raylib.

Os colliders não precisam ser desenhados no jogo final. Porém, deve existir modo debug para desenhar caixas dos colliders.

### Não usar mesh visual como colisão no MVP

A primeira versão deve usar apenas box colliders (`COL_`).

Mesh collision pode ser avaliada depois, mas não é necessária para começar e tende a complicar a física.

### Transform global

Se os objetos estiverem dentro de coleções/parents no Blender, o loader deve calcular transform global, não apenas local.

### Escala

A escala do Blender precisa ser validada com o carro atual.

Sugestão:

- 1 unidade = 1 metro;
- testar com uma pista simples de 20m x 100m;
- ajustar se necessário.

## Ordem recomendada para o MVP

1. Criar uma pista mínima no Blender:
   - plano visual;
   - `COL_ROAD_001`;
   - `SPAWN_PLAYER`.
2. Exportar para `resources/levels/test_track/test_track.glb`.
3. Implementar `LoadLevel()` visual.
4. Integrar `cgltf` e listar nodes.
5. Ler `SPAWN_PLAYER` e posicionar carro.
6. Ler `COL_` e criar colliders.
7. Adicionar checkpoints e finish line.

## Resultado esperado

Ao final da implementação, o fluxo será:

```txt
Blender
  ↓ exporta .glb
resources/levels/<fase>/<fase>.glb
  ↓
DragonRage carrega fase
  ↓
visual + colisão + spawn + checkpoints + finish line
```

Assim, para adicionar uma fase nova, basta:

1. criar a pista no Blender seguindo as convenções;
2. exportar `.glb`;
3. colocar em `resources/levels/<fase>/`;
4. selecionar/carregar a fase no jogo.
