# Auditoria técnica sênior — DragonRageRaylib

Data: 2026-06-08  
Escopo: código C++/Raylib/Jolt em `src/`, configs/assets em `resources/`, build/docs/scripts principais.  
Observação: análise estática/manual. Não executei build nem profiling runtime.

---

## Visão geral

O projeto está em estágio de protótipo funcional, com boa intenção de modularização e bons sinais de pipeline próprio: níveis em GLB, configs JSON, editor/debug in-game, Jolt para colisão de personagem, shader de fog/iluminação e tooling para ajuste rápido.

Porém, a arquitetura já mostra sintomas fortes de crescimento orgânico sem fronteiras claras entre engine, gameplay, editor, assets, física, render e UI. O risco principal não é Raylib. O risco principal é a base virar um bloco monolítico difícil de evoluir, testar e depurar.

A decisão de usar Raylib continua viável para visual PS2-like. A arquitetura atual, não. Ela suporta protótipo, mas precisa de refatoração antes de crescer em gameplay, veículos, levels maiores, mais NPCs, save/load, UI real ou ferramentas de produção.

---

## Arquitetura geral

### Estado atual

O fluxo principal é simples:

- `src/main.cpp`
  - cria janela
  - carrega `GameWorld`
  - loop: `UpdateGameplay()` + `DrawGameplay()`
- `src/game/gameWorld.hpp`
  - agrega quase todo estado global do jogo
- `src/gameplay/worldActions.cpp`
  - carrega/descarrega mundo, níveis, configs, física, NPCs, shaders, props
- `src/gameplay/gameplayUpdate.cpp`
  - atualização do personagem, diálogo, física, partículas, interação
- `src/gameplay/gameplayRender.cpp`
  - render 3D, UI, debug/editor overlays

### Pontos positivos

- Existe separação inicial por domínio (`level`, `gameplay`, `physics`, `render`, `input`, `entity`, `editor`, `debug`).
- Configs externas existem (`resources/config/*.json`, `resources/levels/*.json`).
- O projeto não está 100% hardcoded; há tentativa clara de pipeline data-driven.
- Jolt foi integrado para colisão real do personagem.
- Editor/debug in-game já acelera iteração.
- Docs de pipeline/visual existem, o que é muito bom para projeto solo.

### Problema central

`GameWorld` virou um **God Object**. Quase todo sistema lê/escreve diretamente nele. Isso cria acoplamento alto, efeitos colaterais difíceis de rastrear e torna refatorações futuras caras.

---

## Organização de pastas, módulos e responsabilidades

A organização por pasta é boa como intenção, mas as responsabilidades vazam:

- `editor/editorDraw.cpp` conhece `GameWorld`, `LevelData`, `EntityRegistry`, `SmokingConfig`, `Props`, `Lighting`, `WorldActions`, `Input`, `DebugIcons`.
- `gameplay/worldActions.cpp` carrega nível, física, shader, UI/debug state, NPCs, player, configs e partículas.
- `entity/entityRegistry.cpp` faz CRUD de entidades, foco/interação, colisão manual, render e UI de diálogo no mesmo módulo.
- `level/levelLoader.cpp` faz parsing GLB, parsing JSON, extração de colisores, debug nodes, skybox, render e manipulação de transform.
- `interactionSystem.cpp` mistura config de personagem, parser JSON, parser GLB, render metadata e colisores.

Isso impede trocar uma parte sem mexer em várias outras.

---

## Problemas encontrados

### 1. `GameWorld` como God Object

**Onde:** `src/game/gameWorld.hpp`, usado em `gameplay`, `editor`, `debug`, `render`, `entity`.

**Por que é problema:** quase todos os sistemas acessam e mutam tudo. Não existe API clara de ownership nem fluxo de dados previsível.

**Impacto atual:** mudanças pequenas exigem entender vários módulos. Debug/editor e runtime se contaminam.

**Impacto futuro:** features como savegame, cutscenes, múltiplos níveis, veículos, multiplayer ou replay ficam muito difíceis.

**Melhor solução:** separar `GameWorld` em serviços/sistemas com APIs explícitas:

- `LevelSystem`
- `RenderSystem`
- `PhysicsSystem`
- `InputSystem`
- `InteractionSystem`
- `DialogueSystem`
- `EditorSystem`
- `AssetManager`

`GameWorld` deveria ser composição leve, não estado público gigante.

**Status atual:** 🟨 Parcial — `UpdateGameplay()` e `DrawGameplay()` já têm contextos menores (`GameplayUpdateContext`/`GameplayRenderContext`) e mantêm wrappers compatíveis. Ainda restam dependências diretas importantes em `worldActions.cpp`, editor/debug e painéis UI.

**Prioridade:** Alta

---

### 2. `DebugUiState` gigante e manual

**Onde:** `src/game/gameWorld.hpp`, struct `DebugUiState`.

**Por que é problema:** dezenas de flags, ids, buffers e estados de drag no mesmo struct. Inicialização manual enorme em `LoadGameWorld()`.

**Impacto atual:** fácil esquecer campo novo, criar estado inconsistente ou bug visual.

**Impacto futuro:** editor cresce e vira impossível manter.

**Melhor solução:** dividir em estados menores:

- `TopBarState`
- `InspectorState`
- `LevelConfigPanelState`
- `PersonPanelState`
- `GizmoState`
- `TeleportPanelState`

Usar construtores/default member initializers. Evitar inicialização posicional gigante.

**Prioridade:** Alta

---

### 3. JSON parsing duplicado e frágil

**Onde:**

- `src/level/levelLoader.cpp`
- `src/level/levelRuntimeConfig.cpp`
- `src/interactionSystem.cpp`
- `src/personController.cpp`
- `src/vehiclePhysics.cpp`
- `src/input/inputMap.cpp`
- `src/game/worldConfig.cpp`
- `src/gameplay/smoking/smokingConfig.cpp`
- `src/level/levelsConfig.cpp`

**Por que é problema:** existem múltiplos parsers caseiros diferentes, alguns completos, outros por `find`, `strtof`, `sscanf`. Comportamento varia por arquivo.

**Impacto atual:** configs inválidas falham silenciosamente, strings escapadas/nested arrays podem quebrar, validação quase não existe.

**Impacto futuro:** pipeline de assets/config vira fonte de bugs difíceis. Ferramenta salva JSON que outro parser pode não ler do mesmo jeito.

**Melhor solução:** usar biblioteca única (`nlohmann/json`, `yyjson`, `simdjson`, `json-c`) e criar camada `ConfigLoader` com validação, logs e defaults.

**Prioridade:** Crítica

---

### 4. Parser GLB duplicado

**Onde:** `src/level/levelLoader.cpp` e `src/interactionSystem.cpp`.

**Por que é problema:** dois parsers de GLB/JSON metadata com lógica parecida para nodes, matrices, bounds, transform hierárquico.

**Impacto atual:** bugs corrigidos em um parser podem continuar no outro.

**Impacto futuro:** animação, materiais, skinning, LOD, tags e pivôs vão exigir duplicar ainda mais código.

**Melhor solução:** extrair `GltfMetadataReader`/`AssetMetadata` comum. Ideal: usar tinygltf/cgltf para metadata e manter Raylib só para GPU model.

**Prioridade:** Alta

---

### 5. Render de partes do nível ignora transform do node

**Onde:** `src/level/levelLoader.cpp`, função `DrawLevel()`.

**Problema:** quando `level.renderParts` existe, o loop chama:

```cpp
DrawMesh(level.visualModel.meshes[part.meshIndex], level.visualModel.materials[materialIndex], rootTransform);
```

`part.transform` é ignorado.

**Por que é problema:** os transforms extraídos de nodes `VISUAL_*` não são aplicados. Se o GLB depende de transform de nodes, meshes podem aparecer no lugar errado/colapsadas no root.

**Impacto atual:** editor visual e runtime podem divergir do GLB real.

**Impacto futuro:** pipeline Blender fica frágil; qualquer level modular com múltiplos nodes pode renderizar errado.

**Melhor solução:** usar `MatrixMultiply(part.transform, rootTransform)` ou voltar para `DrawModelEx` quando não precisar draw por mesh. Padronizar: ou render por node com transform correto, ou render por model inteiro e usar nodes só para metadata.

**Prioridade:** Crítica

---

### 6. Branch morto/confuso para visual transform

**Onde:** `src/level/levelLoader.cpp`, função `ApplyLevelDebugNodeTransform()`.

**Problema:** a função retorna imediatamente se `node.kind == Visual`, mas depois contém lógica para `Visual` dentro da mesma função.

**Por que é problema:** indica refatoração incompleta. Código morto confunde manutenção.

**Impacto atual:** editor mostra visual read-only, mas código sugere suporte parcial.

**Impacto futuro:** dev pode tentar reativar visual editing e quebrar render/physics.

**Melhor solução:** remover código morto ou implementar visual transform corretamente com regra clara.

**Prioridade:** Média

---

### 7. Física Jolt e colisão manual fora de sincronia

**Onde:** `src/gameplay/gameplayUpdate.cpp`, `ResolveCharacterCollisions()` em `src/entity/entityRegistry.cpp`, `JoltWorld::UpdateCharacter()`.

**Problema:** Jolt atualiza o personagem e escreve `person.position`. Depois `ResolveCharacterCollisions()` altera `person.position` manualmente, mas não atualiza o `CharacterVirtual` no Jolt.

**Por que é problema:** na próxima simulação, Jolt pode sobrescrever a correção. NPCs não existem como corpos de colisão reais.

**Impacto atual:** colisão com NPC pode tremer, falhar ou parecer funcionar só visualmente.

**Impacto futuro:** qualquer física mais séria fica inconsistente.

**Melhor solução:** registrar NPC colliders no Jolt como bodies/sensors ou após correção chamar `SetCharacterPosition()`. Melhor ainda: uma única fonte de verdade para colisão.

**Prioridade:** Alta

---

### 8. Teleporte do jogador não atualiza Jolt

**Onde:** `src/editor/editorDraw.cpp`, função `TeleportPerson()`.

**Problema:** altera `gameWorld.player.state.position`, mas não chama `gameWorld.player.physics->SetCharacterPosition()`.

**Por que é problema:** no próximo update físico, Jolt provavelmente restaura posição interna antiga.

**Impacto atual:** teleporte pode parecer não funcionar ou funcionar só por um frame.

**Impacto futuro:** qualquer comando externo que move player terá bug semelhante.

**Melhor solução:** criar API única `PlayerSystem::Teleport(position)` que atualiza estado visual, câmera e física.

**Prioridade:** Alta

---

### 9. Acumulador de física pode acumular backlog

**Onde:** `src/gameplay/gameplayUpdate.cpp`, loop `while (physicsAccumulator >= physicsStep && steps < 8)`.

**Problema:** se `steps` chega em 8, o resto do acumulador continua. Em queda de FPS, backlog pode persistir e causar slow-motion/espiral.

**Impacto atual:** stutter em máquinas lentas ou carregamentos.

**Impacto futuro:** cenas maiores pioram.

**Melhor solução:** após cap de steps, descartar/clamp acumulador:

```cpp
if (steps == maxSteps) accumulator = 0.0f;
```

ou usar fixed timestep com interpolação e métricas.

**Prioridade:** Média

---

### 10. `worldActions.cpp` concentra lifecycle demais

**Onde:** `src/gameplay/worldActions.cpp`.

**Por que é problema:** carrega recursos, reinicia level, salva config, aplica shader, reseta player, mexe em UI e física. Mistura runtime, editor e IO.

**Impacto atual:** duplicação entre `RestartLevel()` e `LoadConfiguredLevel()`.

**Impacto futuro:** loading async, streaming ou múltiplos mapas ficam caros.

**Melhor solução:** separar:

- `WorldLoader`
- `LevelManager`
- `PlayerSpawner`
- `RenderResourceBinder`
- `EditorPersistence`

**Prioridade:** Alta

---

### 11. Código morto/legado significativo

**Onde:**

- `src/vehiclePhysics.*` — não usado em nenhum fluxo atual.
- `src/staticWorld.*` — carregado/descarregado, mas não desenhado.
- `src/scenery.*`, `src/roads.*`, `src/ui.*` — não usados no fluxo atual.
- `distanceTraveled` em `src/gameState.*` — global não usado.
- câmeras cockpit/chase em `src/debug/cameraDebug.*` — parcialmente sem uso.

**Por que é problema:** aumenta custo mental e risco de mexer em sistema morto achando que é ativo.

**Impacto atual:** confusão arquitetural.

**Impacto futuro:** base vira depósito de protótipos antigos.

**Melhor solução:** mover para `src/legacy/` fora do build, apagar, ou reativar com testes e ownership claro.

**Prioridade:** Média

---

### 12. Assets referenciados não existem

**Onde:**

- `src/staticWorld.cpp` usa `resources/models/Building.glb`.
- `src/scenery.cpp` usa `resources/models/Building.glb`.
- `src/roads.cpp` usa `resources/models/RoadAsphalt.glb` e `resources/textures/ground/sand_3.png`.

No repositório atual existem `resources/textures/Building.png`, mas não `resources/models/Building.glb` nem `resources/models/RoadAsphalt.glb`.

**Por que é problema:** sistemas antigos podem falhar em runtime se reativados. `staticWorld` ainda carrega no boot.

**Impacto atual:** warnings, possível model inválido; dependendo do Raylib/materialCount pode causar acesso inválido.

**Impacto futuro:** empacotamento quebrado e builds que “funcionam na máquina” mas não em outra.

**Melhor solução:** validar assets no carregamento antes de acessar materiais. Remover sistemas mortos ou corrigir paths.

**Prioridade:** Alta

---

### 13. Sem AssetManager/cache

**Onde:** `LoadRuntimeProps()`, `LoadEntityRegistry()`, `LoadLevel()`, `ParticleSystem::CreateEmitter()`, `LoadStaticWorld()`.

**Por que é problema:** cada prop/personagem carrega seu próprio `Model`/`Texture`. Duplicatas não são compartilhadas.

**Impacto atual:** aceitável em cena pequena.

**Impacto futuro:** memória e loading explodem com props repetidos, NPCs repetidos, partículas múltiplas.

**Melhor solução:** `AssetManager` com cache por path, ref-count/handles e unload centralizado. Separar CPU metadata de GPU resource.

**Prioridade:** Alta

---

### 14. Recursos Raylib copiados por valor sem RAII

**Onde:** `InteractableCharacter`, `RuntimeProp`, `StaticWorld`, `LevelData`, `ParticleEmitter`.

**Por que é problema:** `Model`, `Texture`, `Shader` são handles para recursos GPU. Structs copyable podem gerar double unload, leaks ou ownership ambíguo.

**Impacto atual:** código depende de disciplina manual.

**Impacto futuro:** refactors com `std::vector`/return/copy podem introduzir bugs graves.

**Melhor solução:** criar wrappers move-only:

- `ModelHandle`
- `TextureHandle`
- `ShaderHandle`

Desabilitar copy, permitir move, unload no destrutor.

**Prioridade:** Alta

---

### 15. `EntityRegistry` mistura storage, gameplay, render e UI

**Onde:** `src/entity/entityRegistry.cpp`.

**Por que é problema:** registry deveria armazenar entidades. Hoje também faz foco, diálogo, colisão, render e desenho de UI.

**Impacto atual:** diálogo depende diretamente de input e drawing; difícil testar.

**Impacto futuro:** branching de quests, localization, escolhas com efeitos, cutscenes e UI real ficarão acoplados.

**Melhor solução:** separar:

- `EntityRegistry` só storage/ids
- `InteractionSystem` calcula foco
- `DialogueSystem` controla diálogo
- `DialogueUI` desenha
- `NpcCollisionSystem` ou Jolt bodies

**Prioridade:** Alta

---

### 16. IDs paralelos em arrays separadas

**Onde:** `EntityRegistry` contém `std::vector<EntityId> characterIds` e `std::vector<InteractableCharacter> characters`.

**Por que é problema:** arrays paralelas podem desalinhar. Funções indexam assumindo tamanho igual.

**Impacto atual:** baixo, enquanto só CRUD local usa corretamente.

**Impacto futuro:** remoção/ordenação/filtros podem quebrar.

**Melhor solução:** `struct EntityRecord { EntityId id; InteractableCharacter character; }` ou sparse set/ECS simples.

**Prioridade:** Média

---

### 17. Estado global legado

**Onde:** `src/gameState.hpp/cpp`.

**Problema:** `extern SysState sysState`, `extern float distanceTraveled`.

**Por que é problema:** estado global mutável fora de `GameWorld` quebra previsibilidade e dificulta testes.

**Impacto atual:** `sysState` é usado pouco; `distanceTraveled` parece morto.

**Impacto futuro:** menus, pause, dialogue e editor podem entrar em estados conflitantes.

**Melhor solução:** `GameStateMachine` dentro de mundo/app, com transições explícitas e eventos.

**Prioridade:** Média

---

### 18. Editor monolítico e duplicação de widgets

**Onde:**

- `src/editor/editorDraw.cpp` (~1400 linhas)
- `src/debug/ui/debugWidgets.cpp`

**Problema:** `editorDraw.cpp` redefine `DebugMenuItem`, `DebugButton`, `DebugFloatSlider`, `DebugTextInput`, `BeginDebugPanel`, etc., já existentes em `debugWidgets.cpp`.

**Por que é problema:** duas implementações visuais e comportamentais para a mesma coisa.

**Impacto atual:** inconsistência e manutenção duplicada.

**Impacto futuro:** UI do editor vira maior gargalo de engenharia.

**Melhor solução:** usar só `debugWidgets.*`, extrair painéis menores:

- `EditorInspectorPanel`
- `EditorGizmo`
- `EditorAddMenu`
- `EditorLightingPanel`
- `EditorPropPanel`

**Prioridade:** Alta

---

### 19. Inconsistência de idioma e nomenclatura

**Onde:** mensagens/UI/docs/código misturam português e inglês (`Person`, `Level`, `Interagir`, `Sem dados`, `Smoking`, `Props`, `Game Teleport`).

**Por que é problema:** não quebra runtime, mas reduz clareza de equipe e consistência de UI/tooling.

**Impacto atual:** baixo.

**Impacto futuro:** localização, documentação e onboarding sofrem.

**Melhor solução:** inglês para código/API; português ou inglês consistente para UI final. Debug pode ser inglês técnico.

**Prioridade:** Baixa

---

### 20. Input acoplado diretamente ao Raylib

**Onde:** `inputMap.cpp`, `personController.cpp`, `entityRegistry.cpp`, `debug/cameraDebug.cpp`, `editorDraw.cpp`.

**Por que é problema:** sistemas chamam `IsKeyDown`, `IsMouseButtonPressed`, `GetMouseDelta` diretamente.

**Impacto atual:** rápido para protótipo.

**Impacto futuro:** remapping completo, gamepad, pause, UI focus, replay/network determinism ficam difíceis.

**Melhor solução:** `InputSystem` gera snapshot por frame (`InputFrame`) e actions/axes. Gameplay consome snapshot, editor consome camada própria.

**Prioridade:** Média

---

### 21. Sem sistema de eventos

**Onde:** arquitetura geral.

**Por que é problema:** sistemas chamam uns aos outros diretamente. Ex.: editor chama `ReloadJoltLevelPhysics()`, world actions chamam render shader binding, gameplay muda `sysState`.

**Impacto atual:** dependências crescentes.

**Impacto futuro:** triggers, quests, audio, UI, save, cutscenes e analytics vão virar chamadas cruzadas.

**Melhor solução:** event bus simples:

- `LevelLoaded`
- `PlayerTeleported`
- `DialogueStarted`
- `DialogueEnded`
- `ConfigChanged`
- `EntityAdded/Removed`

**Prioridade:** Média

---

### 22. Triggers/checkpoints/sensores incompletos

**Onde:** `LevelData` tem triggers/checkpoints/finish line; `JoltWorld::LoadLevel()` cria sensor bodies; não há callbacks/consulta de overlap usados.

**Por que é problema:** pipeline indica gameplay de corrida, mas runtime não implementa fluxo.

**Impacto atual:** funcionalidade parece existir, mas não funciona como gameplay.

**Impacto futuro:** track/race logic terá que ser reconstruída.

**Melhor solução:** implementar `TriggerSystem` com callbacks Jolt ou queries por frame, estado de corrida e testes.

**Prioridade:** Média

---

### 23. Sistema de iluminação declara shadows, mas renderer não implementa sombras

**Onde:** `LevelRuntimeConfig`, `levelConfigSidebar.cpp`, `fogRenderer.cpp`, `fog_lit.fs`.

**Problema:** configs têm `shadowsEnabled`, `shadowResolution`, `shadowBias`, `castShadows`, mas shader/render pass não tem shadow map.

**Por que é problema:** opção salva no editor dá falsa expectativa.

**Impacto atual:** confusão; config parece ter efeito que não existe.

**Impacto futuro:** dívida de renderer cresce.

**Melhor solução:** marcar no UI como “not implemented” de forma mais forte ou esconder até existir shadow pipeline. Para PS2-like, considerar baked shadows/lightmaps antes de shadow map real.

**Prioridade:** Média

---

### 24. Hard limits de luzes sem feedback

**Onde:** `FogShader::MaxDirectionalLights = 2`, `MaxPointLights = 4`, `MaxSpotLights = 2`; shader igual.

**Por que é problema:** luzes além do limite são ignoradas silenciosamente.

**Impacto atual:** cena pequena ok.

**Impacto futuro:** editor permite criar luz que não renderiza.

**Melhor solução:** UI deve mostrar limite, bloquear criação ou marcar luz ignorada. Renderer deve logar quando truncar.

**Prioridade:** Média

---

### 25. Sem culling/LOD/batching/instancing

**Onde:** `DrawLevel()`, `DrawRuntimeProps()`, `DrawEntityCharacters()`, `ParticleSystem::DrawParticles()`.

**Por que é problema:** tudo é desenhado se carregado. Sem frustum culling, distance culling, LOD ou instancing.

**Impacto atual:** aceitável para mapa pequeno PS2-like.

**Impacto futuro:** props repetidos/NPCs/partículas vão custar draw calls e fillrate.

**Melhor solução:** começar simples:

- bounding sphere/AABB por renderable
- frustum + distance culling
- cache de materiais/texturas
- batching/instancing para props repetidos
- limite de partículas por emitter e por frame

**Prioridade:** Média

---

### 26. Partículas usam busca linear por partícula livre

**Onde:** `src/particles/particleSystem.cpp`, `UpdateParticles()`.

**Por que é problema:** para cada spawn, varre pool até achar inativa.

**Impacto atual:** ok com poucos particles.

**Impacto futuro:** fumaça, poeira de carro, chuva, faíscas e efeitos múltiplos podem gerar custo O(spawns * pool).

**Melhor solução:** freelist/ring buffer por emitter.

**Prioridade:** Baixa/Média

---

### 27. `ParticleSystem::CreateEmitter()` usa `std::move` sem incluir `<utility>`

**Onde:** `src/particles/particleSystem.cpp`.

**Por que é problema:** pode compilar por include indireto, mas não é garantido pelo padrão.

**Impacto atual:** possível fragilidade em compiladores/plataformas.

**Melhor solução:** incluir `<utility>`.

**Prioridade:** Baixa

---

### 28. Config save não é atômico e não preserva comentários/formatação

**Onde:** `SaveLevelRuntimeConfig()`, `SavePersonConfig()`, `SaveSmokingConfig()`, `SaveLevelsConfig()`.

**Por que é problema:** se app fecha durante escrita, arquivo pode corromper. Também reescreve JSON inteiro.

**Impacto atual:** risco baixo, mas editor salva configs importantes.

**Impacto futuro:** perda de dados de level/editor.

**Melhor solução:** escrever em `.tmp`, validar, depois rename atômico. Fazer backup `.bak` para configs editáveis.

**Prioridade:** Média

---

### 29. Falta validação de schema/config

**Onde:** todos loaders de config.

**Por que é problema:** valores inválidos são aceitos ou caem em fallback silencioso. Ex.: `fixed_time_step <= 0`, escalas negativas, `fog.end <= fog.start`, paths vazios, ranges de luz inválidos.

**Impacto atual:** bugs de tuning parecem bugs de engine.

**Impacto futuro:** editor e assets de produção precisam feedback claro.

**Melhor solução:** validação central com logs de erro e defaults explícitos. Ideal: JSON schema ou validators por config.

**Prioridade:** Alta

---

### 30. Build sem warnings/sanitizers/tests

**Onde:** `CMakeLists.txt`, `.github/workflows/build-macos.yml`.

**Problema:** não há flags como `-Wall -Wextra -Wpedantic`, sanitizers opcionais, testes automatizados, clang-format, clang-tidy ou CI Linux/Windows.

**Impacto atual:** warnings e UB podem passar despercebidos.

**Impacto futuro:** cross-platform quebra tarde.

**Melhor solução:** adicionar targets/options:

- `DRAGONRAGE_WARNINGS_AS_ERRORS`
- `DRAGONRAGE_ASAN`
- `DRAGONRAGE_UBSAN`
- CI macOS + Linux + Windows
- testes unitários para config parsers, math, transform, loading metadata

**Prioridade:** Alta

---

### 31. `CMakeLists.txt` usa `GLOB_RECURSE`

**Onde:** `CMakeLists.txt`.

**Por que é problema:** `CONFIGURE_DEPENDS` reduz dor, mas build explícito é melhor para projeto comercial. Arquivos mortos entram automaticamente no binário.

**Impacto atual:** sistemas legacy continuam compilando.

**Impacto futuro:** aumenta tempo de build e risco de conflitos.

**Melhor solução:** listar fontes por módulo ou usar targets/libraries por pasta.

**Prioridade:** Baixa/Média

---

### 32. Main força fullscreen por resize manual

**Onde:** `src/main.cpp`.

**Problema:** lê `world.json`, cria janela, depois redimensiona para monitor atual e posição `(0,0)`.

**Por que é problema:** config de `window_width/height` perde efeito em monitor válido. Não usa fullscreen/window flags de forma explícita.

**Impacto atual:** pode surpreender dev/user.

**Impacto futuro:** opções gráficas, multi-monitor e windowed mode ficam ruins.

**Melhor solução:** config com modo explícito: `windowed`, `borderless`, `fullscreen`, resolução desejada.

**Prioridade:** Baixa/Média

---

## Avaliação específica de jogos

### Gameplay systems

Atual foco é personagem em primeira pessoa, diálogo simples, fumar/partículas e editor. Sistemas de corrida/veículo existem no código, mas não integrados. Para um jogo de corrida ou exploração maior, falta camada clara de gameplay state, objectives, triggers, progression e rules.

**Risco:** gameplay virar scripts espalhados em `gameplayUpdate`, `entityRegistry` e editor.

### Gerenciamento de estado

`SysState` global é insuficiente. Debug UI pausa física com retorno antecipado em `UpdateGameplay()`, diálogo pausa física, mas não há state machine formal.

**Recomendação:** `GameMode`/`GameStateMachine` com transições e policies:

- `Playing`
- `Paused`
- `Dialogue`
- `Editor`
- `Loading`

### Eventos

Não existe event bus. Hoje sistemas chamam funções diretamente.

**Recomendação:** implementar eventos simples antes de triggers/quests/audio.

### Persistência

Existe persistência de configs/editor. Não existe savegame. Escritas não são atômicas.

**Recomendação:** separar `EditorPersistence` de `GameSave`. Não misturar config de level com progresso do jogador.

### Networking

Não há networking. Arquitetura atual não é preparada para determinismo ou replicação, pois input/render/física/estado global estão acoplados.

**Recomendação:** ignorar networking por enquanto. Se for futuro real, começar desacoplando input snapshot, fixed update e state replication.

### UI

Debug UI é immediate-mode manual. Serve para ferramenta interna. UI de diálogo está dentro de `EntityRegistry`.

**Recomendação:** separar UI final de debug UI. Criar camada `GameUI` para HUD/diálogo e `EditorUI` para ferramentas.

### Assets/pipeline

Pipeline por GLB com nomes `VISUAL_`, `COL_*`, `SPAWN_*`, `TRIGGER_*` é bom para projeto solo. Mas parser é caseiro, duplicated e sem schema.

**Recomendação:** formalizar pipeline:

- validador CLI de GLB/level
- relatório de nodes inválidos
- docs com convenções obrigatórias
- asset registry/cache
- testes de metadata loader

### Render/visual PS2

Shader de fog/lighting quantizado combina com estética PS2. Falta render em baixa resolução/upscale, texture filtering padronizado, dithering/post-process, material system simples e controle de draw distance.

**Recomendação:** criar `Renderer` com render target interno para low-res upscale e post-process. Não espalhar calls Raylib por gameplay/editor.

---

## Oportunidades de refatoração prioritárias

### Fase 1 — estabilização técnica

1. Corrigir `DrawLevel()` para aplicar `part.transform`.
2. Corrigir teleporte/player-Jolt sync.
3. Remover ou isolar código morto (`vehiclePhysics`, `staticWorld`, `roads`, `scenery`) do build.
4. Adicionar warnings fortes e CI Linux/macOS.
5. Criar JSON parser único.
6. Validar assets no load antes de acessar materiais.

### Fase 2 — arquitetura base

1. Criar `AssetManager`.
2. Criar wrappers RAII move-only para `Model`, `Texture`, `Shader`.
3. Separar `EntityRegistry`, `DialogueSystem`, `InteractionSystem`, `DialogueUI`.
4. Quebrar `editorDraw.cpp` em módulos menores.
5. Criar `GameStateMachine`.

### Fase 3 — crescimento de jogo

1. Implementar `TriggerSystem` real.
2. Implementar race/gameplay objectives.
3. Criar renderer com render target low-res, post-process e culling.
4. Criar asset validation CLI.
5. Criar savegame separado de config.

---

## Viabilidade da arquitetura atual para crescimento

### Curto prazo

Boa para protótipo e experimentação visual. Dá para continuar testando estética PS2, level authoring, câmera, interação e física básica.

### Médio prazo

Risco alto. Editor, loading, física e entidades já estão acoplados demais. Sem refatorar, cada feature nova aumenta complexidade não linear.

### Longo prazo

Não recomendada sem reestruturação. Para produção comercial ou jogo maior, dívida técnica atual vai travar velocidade.

Raylib é viável. O gargalo provável será arquitetura própria, não biblioteca.

---

## Tabela rápida de prioridades

| Prioridade | Problema | Status |
|---|---|---|
| Crítica | JSON parser duplicado/frágil | ✅ Feito parcial — parser comum em `src/assets/json.*`; configs runtime e GLB metadata principais migrados. Restam legacy/debug (`vehiclePhysics`, `cameraDebug`). |
| Crítica | `DrawLevel()` ignora `part.transform` | ✅ Feito — `DrawLevel()` agora combina `part.transform` com `rootTransform` antes de `DrawMesh()`. |
| Alta | `GameWorld` God Object | 🟨 Parcial — update/render usam contextos menores com wrappers; restam `worldActions.cpp`, editor/debug e UI dependentes de `GameWorld&`. |
| Alta | `DebugUiState` gigante | ⬜ Pendente |
| Alta | Lifecycle concentrado em `worldActions.cpp` | ⬜ Pendente |
| Alta | Física Jolt e colisão manual fora de sincronia | ✅ Feito — sync explícito após colisão manual via `SetCharacterPosition()`. Ideal futuro: NPCs como bodies/sensors Jolt. |
| Alta | Teleporte não atualiza Jolt | ✅ Feito — `TeleportPerson()` sincroniza posição/velocidade Jolt e zera acumulador. |
| Alta | Sem AssetManager/cache | ⬜ Pendente |
| Alta | Recursos Raylib sem RAII | ⬜ Pendente |
| Alta | `EntityRegistry` mistura storage/gameplay/render/UI | ⬜ Pendente |
| Alta | Editor monolítico e widgets duplicados | ⬜ Pendente |
| Alta | Falta validação de configs/assets | 🟨 Parcial — parse inválido gera warning/default em configs migradas; ainda sem schema/validador completo. |
| Alta | Build sem warnings/sanitizers/testes | ⬜ Pendente |
| Média | Código morto/legado | ⬜ Pendente |
| Média | Triggers/checkpoints incompletos | ⬜ Pendente |
| Média | Shadows configurados mas não implementados | ⬜ Pendente |
| Média | Acumulador de física backlog | ⬜ Pendente |
| Média | Estado global `sysState` | ⬜ Pendente |
| Média | Input acoplado ao Raylib | ⬜ Pendente |
| Média | Sem event bus | ⬜ Pendente |
| Média | Sem culling/LOD/batching | ⬜ Pendente |
| Baixa | Inconsistência idioma/nomenclatura | ⬜ Pendente |
| Baixa | `std::move` sem `<utility>` | ⬜ Pendente |
| Baixa/Média | `GLOB_RECURSE` no CMake | ⬜ Pendente |

---

## Resumo Executivo

### Principais pontos positivos

- Boa escolha de Raylib para visual PS2-like/protótipo 3D.
- Integração com Jolt é bom passo técnico.
- Pipeline GLB com convenções de nomes é promissor.
- Configs externas e editor/debug in-game aceleram iteração.
- Shader de fog/lighting já aponta para estética correta.
- Organização de pastas tem intenção modular.
- Documentação de pipeline/visual existe e ajuda muito.

### Principais problemas encontrados

- `GameWorld` ainda é um God Object, mas update/render já foram reduzidos para contextos menores com wrappers compatíveis.
- JSON/GLB parsing duplicado e frágil.
- `editorDraw.cpp` é monolítico e duplica widgets.
- Render de nível ignorava transforms de partes (`part.transform`) — corrigido em `DrawLevel()`.
- Física Jolt e colisões manuais não têm fonte única de verdade.
- Teleporte do player não atualiza Jolt.
- Recursos Raylib não têm RAII/ownership seguro.
- Não existe AssetManager/cache.
- Muito código legado/morto ainda no build.
- Configs e assets não têm validação robusta.
- Build/CI não têm warnings fortes, sanitizers ou testes.

### Riscos técnicos

- Crescimento do projeto criar regressões constantes.
- Editor/debug virar dependência inseparável do runtime.
- Assets duplicados consumirem memória e loading.
- Bugs de física por dessincronização Jolt/estado C++.
- Pipeline Blender/GLB quebrar silenciosamente.
- Configs inválidas causarem comportamento estranho sem erro claro.

### Dívida técnica acumulada

Média-alta para estágio de protótipo. Ainda controlável, mas precisa ser paga antes de adicionar sistemas grandes como veículo final, corrida completa, saves, UI final, quests, múltiplos mapas ou mais NPCs.

### O que corrigir primeiro

1. Corrigir `DrawLevel()` usando `part.transform`. ✅ Feito — renderParts aplicam transform do node GLB + root do level.
2. Corrigir teleporte e colisão manual para sincronizar com Jolt.
3. Adotar JSON parser único e validação de config.
4. Remover/isolar código morto do build.
5. Criar AssetManager + RAII para `Model/Texture/Shader`.
6. Quebrar `editorDraw.cpp` e remover widgets duplicados.
7. Separar `EntityRegistry` de diálogo/render/UI.
8. Adicionar warnings fortes, CI Linux/macOS e testes básicos.

### Nota geral do projeto

**5.2 / 10**

Como protótipo: promissor.  
Como base de produção: frágil.

### Avaliação do nível do código

**Pleno em prototipagem, abaixo de Produção Comercial.**

Há boas decisões e iniciativa técnica, mas a base ainda tem padrões de projeto solo/protótipo: ownership manual, parsers duplicados, acoplamento alto, sistema de estado fraco e editor monolítico. Com refatoração focada, pode evoluir bem.
