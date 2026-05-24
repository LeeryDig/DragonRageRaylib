# Debug Level Editor

## Layout da top bar

```
Game | Level | Inspector | Debug | Person | Physics
```

Abrir `Level` fecha `Inspector` e vice-versa.

## Sidebar: Level

### Tab: Skyboxes

- Lista arquivos de `resources/assets/skybox` (`.png`, `.jpg`, `.jpeg`, `.hdr`, `.ktx`).
- Destaca skybox atual do level.
- Clicar → atualiza `currentLevelRuntimeConfig.skyboxPath` → `SaveCurrentLevelRuntimeConfig()` → reload.

### Tab: Levels

- Lista vinda de `resources/config/levels.json`.
- Destaca level atual.
- Botão `Load` carrega level selecionado.

## Sidebar: Inspector

Lista entidades do level:

```
LEVEL <nome>
CHAR <id>
LIGHT <id> [Directional/Point/Spot]
```

Selecionando CHAR ou LIGHT:
- Campos `Position` / `Rotation` XYZ.
- `Move` / `Rotate` ativam gizmo 3D.
- `Teleport to Camera`.
- `Save level config` → grava no `.json` do level atual.

Para LIGHT: ver [lighting-system.md](lighting-system.md) (IDs de seleção, debug draw, context menu, delete).

## Salvamento: `SaveCurrentLevelRuntimeConfig()`

Serializa para o `.json` do level atual:
- `skyboxPath`
- `fog`
- `lighting.ambient` + `lighting.lights[]`
- `characters[]` com transforms atuais

Qualquer mudança marca `levelConfigDirty = true`.
