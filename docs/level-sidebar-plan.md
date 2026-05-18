# Plano: Sidebar Level

## Objetivo

Criar uma aba única no topo chamada `Level`.
Ao clicar, abre uma sidebar igual ao Inspector, mas focada em configuração do level.

## Estado atual / já mexido

Já comecei mudanças antes do plano. Precisam ser revisadas/normalizadas.

Arquivos já afetados:
- `src/main.cpp`
- `src/game/gameWorld.hpp`
- `src/level/levelRuntimeConfig.hpp`
- `src/level/levelRuntimeConfig.cpp`

Mudanças já feitas na rodada anterior do Inspector:
- Inspector virou aba própria no top bar.
- Inspector abre sidebar ao clicar.
- Inspector ficou com lista principal só `ROOT`.
- Selecionando `CHAR`, mostra Position/Rotation.
- Botões `Move` / `Rotate` ativam gizmo.
- Gizmo 3D básico para mover/rotacionar item selecionado.
- `Save car_meet.json` salva transforms dos characters no json do level atual.
- `SaveLevelRuntimeConfig()` criado para gravar config runtime em disco.
- `LevelRuntimeConfig` já tem suporte a ler/gravar `skybox` e `characters`.

Mudanças já iniciadas para `Level Config` antes deste plano:
- `DebugUiState` ganhou campos:
  - `levelConfigOpen`
  - `levelConfigScroll`
  - `levelConfigTab` começou a ser adicionado
- `main.cpp` ganhou includes:
  - `<filesystem>`
  - `<cctype>`
- Funções auxiliares já adicionadas:
  - `IsSkyboxFile(...)`
  - `ListSkyboxPaths()`
  - `ReloadCurrentLevelForConfig(...)`
- `DrawLevelConfigSidebar(...)` foi criado parcialmente.
- Essa função já lista skyboxes de `resources/assets/skybox`.
- Ao clicar skybox diferente, ela já tenta:
  1. atualizar `currentLevelRuntimeConfig.skyboxPath`
  2. chamar `SaveCurrentLevelRuntimeConfig(...)`
  3. chamar `ReloadCurrentLevelForConfig(...)`
- Ainda precisa ajustar para nome final `Level`, tabs internas, e top bar final.

## Estrutura UI desejada

Top bar final:
- `Game`
- `Level`
- `Inspector`
- `Debug`
- `Person`
- `Physics`

Sidebar `Level`:
- Título: `Level`
- Tabs internas:
  - `Skyboxes`
  - `Levels`

## Tab: Levels

Responsabilidade: carregar/trocar levels.

Funcionalidade:
- Mostrar lista vinda de `resources/config/levels.json`.
- Mostrar level atual destacado.
- Selecionar level da lista.
- Botão `Load` carrega level selecionado.
- Manter botões existentes `Move Up` / `Move Down` se já existirem.
- Não misturar config do level aqui.

Implementação provável:
- Reusar lógica atual de `DrawLevelLoadSidebar(...)`.
- Mover conteúdo para dentro de `DrawLevelConfigSidebar(...)`, tab `Levels`.
- Depois remover uso visual/sidebar separada `levelLoadOpen` ou deixar só como estado legado sem menu.

## Tab: Skyboxes

Responsabilidade: escolher skybox do level atual.

Funcionalidade:
- Listar arquivos da pasta `resources/assets/skybox`.
- Extensões aceitas já começadas:
  - `.png`
  - `.jpg`
  - `.jpeg`
  - `.hdr`
  - `.ktx`
- Destacar skybox atual do level.
- Ao clicar em skybox diferente:
  1. Atualiza `currentLevelRuntimeConfig.skyboxPath`.
  2. Salva imediatamente no `.json` do level atual, ex: `resources/levels/car_meet.json`.
  3. Recarrega o level/skybox para refletir mudança.
- Sem botão extra de salvar.

Observação técnica:
- Salvamento usa `SaveCurrentLevelRuntimeConfig(...)`, que hoje também sincroniza transforms de characters antes de salvar.
- Isso é bom porque não perde alteração já feita no Inspector.

## Fora do escopo agora

Não implementar ainda:
- Seleção/spawn de characters.
- Adicionar luzes.
- Adicionar itens.
- Palette de assets.
- Sistema genérico de entidades.

Isso vem depois como sistema mais amplo de objetos spawnáveis no level.

## Ajustes necessários no código atual

1. Trocar top bar para ter `Level` e não `Load Level` em `Game`.
2. `Game > Load Level` deve sumir ou não abrir sidebar separada.
3. Clicar `Level` abre `levelConfigOpen`.
4. Abrir `Level` fecha `Inspector` (`levelSidebarOpen = false`).
5. Abrir `Inspector` fecha `Level` (`levelConfigOpen = false`).
6. `DrawLevelConfigSidebar(...)` deve ter tabs `Skyboxes` e `Levels`.
7. Chamar `DrawLevelConfigSidebar(gameWorld)` em `DrawGameplay(...)`.
8. Remover/ignorar `DrawLevelLoadSidebar(...)` na renderização final, ou manter só até migrar tudo.
9. Garantir inicialização correta dos novos campos em `DebugUiState`.
10. Garantir que selecionar skybox salva no json e recarrega sem precisar outro botão.

## Observações

- `Inspector` fica para itens existentes dentro do level: mover/rotacionar com gizmo, salvar transform etc.
- `Level` fica para configuração/lista de levels e skybox.
- `Load Level` separado deixa de existir como menu próprio; vira tab `Levels` dentro de `Level`.
