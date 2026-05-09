# Guia para montar pistas/fases no Blender

Este documento define o padrão de criação de pistas/fases no Blender para o DragonRage.

A ideia principal é: **o Blender é a fonte única da fase**. A parte visual, colliders, spawn, checkpoints e finish line devem estar dentro do arquivo `.glb` exportado.

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
COL_ROAD_001
COL_ROAD_002
COL_WALL_LEFT_001
COL_WALL_RIGHT_001
COL_RAMP_001
COL_OBSTACLE_001
```

### Regra inicial

No MVP, existem dois tipos principais:

- `COL_Road_*` e `COL_Ramp_*`: devem ser **planos/retângulos** que formam a superfície dirigível da pista.
- Outros `COL_*`, como `COL_Wall_*` e `COL_Obstacle_*`: são interpretados como **box colliders/paralelepípedos**.

Para roads/ramps, o jogo usa o plano exatamente onde ele foi posicionado/rotacionado no Blender. O tamanho do plano define a área válida de contato da pista.

Para boxes, o jogo usa:

- posição;
- rotação;
- escala;
- dimensões do objeto.

### Boas práticas para colliders

- Use poucos colliders grandes em vez de muitos pequenos.
- Para chão/pista, use planos retangulares `COL_Road_*` encaixados ao longo da pista.
- Para paredes laterais, use boxes/paralelepípedos altos e estreitos.
- Para rampas, use planos retangulares `COL_Ramp_*` rotacionados/inclinados.
- Evite usar mesh visual complexo como collider no começo.
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
| Collider | `COL_` | `COL_ROAD_001` |
| Spawn | `SPAWN_PLAYER` | `SPAWN_PLAYER` |
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
- colliders do tipo box vindos de objetos `COL_`;
- checkpoints vindos de objetos `CHECKPOINT_###`;
- finish line vinda de `FINISH_LINE`.
