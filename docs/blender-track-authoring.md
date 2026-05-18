# Guia para montar pistas/fases no Blender

Este documento define o padrão de criação de pistas/fases no Blender para o DragonRage.

A ideia principal é: **o Blender é a fonte única da fase**. A parte visual, colliders, spawn, checkpoints e finish line devem estar dentro do arquivo `.glb` exportado.

A física de personagem/cenário usa **Jolt Physics**: cenário estático vira static body, player usa capsule character controller.

## Estrutura recomendada da fase

Cada fase deve ficar em uma pasta própria:

```txt
resources/levels/city_01/
  city_01.glb
```

Exemplos:

```txt
resources/levels/
  city_01/
    city_01.glb
  desert_01/
    desert_01.glb
  snow_01/
    snow_01.glb
```

## Organização no Blender

Use coleções para organizar a cena:

```txt
LEVEL_city_01
├── VISUAL
├── COLLIDERS
├── SPAWN
├── CHECKPOINTS
└── FINISH
```

## Objetos visuais

Tudo que for parte visual da fase deve ficar na coleção `VISUAL`.

Exemplos de nomes:

```txt
VISUAL_Track
VISUAL_Buildings
VISUAL_Props
VISUAL_Scenery
VISUAL_Trees
VISUAL_Tunnel
```

Esses objetos serão renderizados normalmente no jogo.

## Colliders

Todos os colliders devem ficar na coleção `COLLIDERS` e começar com o prefixo `COL_`.

Exemplos:

```txt
COL_BOX_WALL_LEFT_001
COL_BOX_BUILDING_BLOCKER_001
COL_WALL_RIGHT_001
COL_OBSTACLE_CRATE_001
COL_ROAD_001
COL_RAMP_001
COL_SURFACE_DIRT_001
COL_MESH_ROCK_001
```

### Regra atual

Existem dois tipos físicos principais:

- `COL_BOX_*`, `COL_WALL_*`, `COL_OBSTACLE_*`: **box colliders/paralelepípedos orientados**.
- `COL_ROAD_*`, `COL_RAMP_*`, `COL_SURFACE_*`, `COL_MESH_*`: **mesh colliders estáticos por triângulos**.

Para boxes, o jogo usa corretamente:

- posição world;
- rotação/ângulo world;
- escala;
- dimensões/bounds do objeto.

Ou seja: cubo alongado vira retângulo, parede fina vira box fino, objeto rotacionado mantém ângulo.

Para mesh/surface, o jogo usa os triângulos reais exportados pelo Blender. Use para chão irregular, pista, rampas, pedras grandes ou formas que não são bem representadas por box.

### Boas práticas para colliders

- Use poucos colliders grandes em vez de muitos pequenos.
- Para chão/pista, use `COL_ROAD_*` ou `COL_SURFACE_*` com malha simples.
- Para paredes laterais, use `COL_BOX_*`/`COL_WALL_*` como boxes altos e estreitos.
- Para rampas, use `COL_RAMP_*` com poucos triângulos.
- Para pedra/forma irregular estática, use `COL_MESH_*` com malha simplificada.
- Não use mesh visual detalhado como collider. Duplique e simplifique.
- Aplique escala/rotação no objeto se o resultado no jogo parecer errado, mas cubo escalado/rotacionado funciona.
- Deixe os colliders em uma coleção separada para poder ocultar/mostrar facilmente.

## Spawn do jogador

Crie um objeto vazio ou mesh simples chamado:

```txt
SPAWN_PLAYER
```

Ele deve ficar na coleção `SPAWN`.

O jogo usará:

- posição do objeto para posicionar o carro;
- rotação do objeto para orientar o carro.

Sugestão: alinhe o eixo local do objeto para apontar na direção inicial da corrida.

## Triggers genéricos

Triggers são volumes de overlap, sem colisão física. Use para dano, evento, troca de música, zona especial etc.

Nome:

```txt
TRIGGER_DAMAGE_001
TRIGGER_EVENT_TUNNEL_001
```

Debug draw de trigger é roxo.

## Checkpoints

Os checkpoints devem ficar na coleção `CHECKPOINTS` e seguir o padrão:

```txt
CHECKPOINT_001
CHECKPOINT_002
CHECKPOINT_003
```

A ordem numérica define a ordem que o jogador deve atravessar.

### Formato recomendado

Use cubos invisíveis/sem render como volume do checkpoint.

O jogo usará:

- posição;
- rotação;
- escala/tamanho;
- número no nome.

## Finish line

A linha de chegada deve ficar na coleção `FINISH` e se chamar:

```txt
FINISH_LINE
```

Ela pode ser um cubo invisível, parecido com checkpoint, ou um objeto visual separado acompanhado de um volume de chegada.

## Convenção de nomes

| Tipo | Prefixo/nome | Exemplo |
|---|---|---|
| Visual | `VISUAL_` | `VISUAL_Track` |
| Box collider | `COL_BOX_`, `COL_WALL_`, `COL_OBSTACLE_` | `COL_BOX_WALL_001` |
| Mesh/surface collider | `COL_ROAD_`, `COL_RAMP_`, `COL_SURFACE_`, `COL_MESH_` | `COL_ROAD_001` |
| Spawn | `SPAWN_PLAYER` | `SPAWN_PLAYER` |
| Trigger genérico | `TRIGGER_` | `TRIGGER_DAMAGE_001` |
| Checkpoint | `CHECKPOINT_###` | `CHECKPOINT_001` |
| Finish | `FINISH_LINE` | `FINISH_LINE` |

## Escala e orientação

Recomendação inicial:

- 1 unidade no Blender = 1 metro no jogo.
- Use eixo Y como vertical no Blender? Atenção: o jogo/raylib usa Y como altura, mas o Blender também usa Z como altura por padrão. A exportação glTF normalmente converte os eixos. Este ponto precisa ser validado na primeira pista de teste.
- Mantenha a pista perto da origem sempre que possível.
- Evite aplicar escalas não uniformes em objetos visuais muito complexos sem testar.

## Exportação para GLB

Exportar em:

```txt
File > Export > glTF 2.0
```

Configuração recomendada:

- Format: `glTF Binary (.glb)`
- Include: Selected Objects ou Scene inteira, conforme organização da fase
- Transform: verificar orientação padrão do glTF
- Geometry: exportar normals e UVs
- Materials: exportar materiais/texturas quando necessário

Arquivo final:

```txt
resources/levels/<nome_da_fase>/<nome_da_fase>.glb
```

Exemplo:

```txt
resources/levels/city_01/city_01.glb
```

## Checklist antes de exportar

- [ ] Existe uma coleção `VISUAL`.
- [ ] Existe uma coleção `COLLIDERS`.
- [ ] Todos os colliders começam com `COL_`.
- [ ] Existe `SPAWN_PLAYER`.
- [ ] Existem checkpoints numerados, se a fase precisar de corrida/checkpoints.
- [ ] Existe `FINISH_LINE`, se a fase tiver final.
- [ ] A escala da pista está coerente com o carro.
- [ ] A direção inicial do carro está correta.
- [ ] O arquivo foi exportado como `.glb`.

## MVP esperado

Na primeira versão da pipeline, a fase deve suportar:

- visual carregado via `.glb`;
- spawn vindo do objeto `SPAWN_PLAYER`;
- colliders box Jolt vindos de `COL_BOX_*`, `COL_WALL_*`, `COL_OBSTACLE_*`;
- static mesh colliders Jolt vindos de `COL_ROAD_*`, `COL_RAMP_*`, `COL_SURFACE_*`, `COL_MESH_*`;
- player andando com capsule character controller do Jolt;
- debug draw: colliders físicos em verde, triggers em roxo;
- checkpoints vindos de objetos `CHECKPOINT_###`;
- finish line vinda de `FINISH_LINE`.
