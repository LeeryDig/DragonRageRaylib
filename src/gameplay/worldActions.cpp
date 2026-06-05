#include "worldActions.hpp"

#include <algorithm>
#include <memory>
#include <string>

#include <raylib.h>
#include <raymath.h>

#include "debug/cameraDebug.hpp"
#include "debug/debugIcons.hpp"
#include "entity/entityRegistry.hpp"
#include "game/gameWorld.hpp"
#include "gameState.hpp"
#include "input/inputMap.hpp"
#include "level/levelLoader.hpp"
#include "level/levelRuntimeConfig.hpp"
#include "level/levelsConfig.hpp"
#include "gameplay/props.hpp"
#include "gameplay/smoking/smokingConfig.hpp"
#include "personController.hpp"
#include "physics/jolt/joltWorld.hpp"
#include "render/fogRenderer.hpp"
#include "staticWorld.hpp"
#include "utils.hpp"

static const char* CAMERA_CONFIG_PATH  = "resources/config/camera.json";
static const char* INPUT_CONFIG_PATH   = "resources/config/input.json";
static const char* PERSON_CONFIG_PATH  = "resources/config/person.json";
static const char* LEVELS_CONFIG_PATH  = "resources/config/levels.json";
static const char* SMOKING_CONFIG_PATH = "resources/config/smoking.json";

static Vector3 EulerDegreesFromQuaternion(Quaternion rotation) {
    Vector3 radians = QuaternionToEuler(rotation);
    return Vector3{radians.x * RAD2DEG, radians.y * RAD2DEG, radians.z * RAD2DEG};
}

void ApplyRuntimeRenderConfig(GameWorld& gameWorld) {
    ApplyFogShaderToModel(gameWorld.world.level.visualModel, gameWorld.render.fogShader);
    ApplyFogShaderToRuntimeProps(gameWorld.world.props, gameWorld.render.fogShader);
    for (int i = 0; i < CharacterCount(gameWorld.npcs); ++i) {
        InteractableCharacter& ch = CharacterAt(gameWorld.npcs, i);
        if (ch.hasModel) ApplyFogShaderToModel(ch.model, gameWorld.render.fogShader);
    }
}

void ReloadJoltLevelPhysics(GameWorld& gameWorld) {
    gameWorld.player.physics->LoadLevel(gameWorld.world.level);
    gameWorld.player.physics->CreateCharacter(gameWorld.player.config, gameWorld.player.state.position);
}

void ResetGameWorld(GameWorld& gameWorld) {
    Vector3 spawnPosition = Vector3{0.0f, 2.0f, 0.0f};
    float spawnYaw = 0.0f;
    if (gameWorld.world.level.playerSpawn.valid) {
        spawnPosition = gameWorld.world.level.playerSpawn.position;
        Vector3 spawnForward = Vector3RotateByQuaternion(
            Vector3{0.0f, 0.0f, -1.0f}, gameWorld.world.level.playerSpawn.rotation);
        spawnYaw = atan2f(spawnForward.x, -spawnForward.z);
    }

    ResetPersonState(gameWorld.player.state, gameWorld.player.config, spawnPosition, spawnYaw);
    gameWorld.player.physics->LoadLevel(gameWorld.world.level);
    gameWorld.player.physics->CreateCharacter(gameWorld.player.config, gameWorld.player.state.position);

    gameWorld.render.debugCamera.enabled = false;
    gameWorld.debugUi.freeCameraActive = false;
    gameWorld.player.physicsAccumulator = 0.0f;
    gameWorld.render.camera = Camera{};
    ApplyPersonCamera(gameWorld.render.camera, gameWorld.player.state, gameWorld.player.config, 1.0f);
    if (gameWorld.npcs.dialogueOpen) {
        gameWorld.npcs.dialogueOpen = false;
        gameWorld.npcs.activeDialogueId = INVALID_ENTITY;
    }
    sysState = SysState::PLAYING;
}

void RestartLevel(GameWorld& gameWorld) {
    gameWorld.particles.Unload();
    gameWorld.player.smoking.emitterHandle = -1;
    UnloadRuntimeProps(gameWorld.world.props);
    UnloadLevel(gameWorld.world.level);
    gameWorld.world.runtimeConfig = LoadLevelRuntimeConfig(gameWorld.world.currentLevelConfigPath);
    gameWorld.debugUi.configDirty = false;
    gameWorld.world.level = LoadLevel(
        gameWorld.world.currentLevelPath, gameWorld.world.runtimeConfig.skyboxPath);
    LoadRuntimeProps(gameWorld.world.props, gameWorld.world.runtimeConfig.props, gameWorld.render.fogShader);
    ApplyRuntimeRenderConfig(gameWorld);
    gameWorld.player.config = LoadPersonConfig(
        Utils::ResolveProjectPath(PERSON_CONFIG_PATH), DefaultPersonConfig());
    gameWorld.player.state = CreatePersonState(gameWorld.player.config);
    DestroyEntityRegistry(gameWorld.npcs);
    gameWorld.npcs = LoadEntityRegistry(gameWorld.world.runtimeConfig.characters);
    ApplyRuntimeRenderConfig(gameWorld);
    ResetGameWorld(gameWorld);
}

void LoadConfiguredLevel(GameWorld& gameWorld, int levelIndex) {
    const LevelConfigEntry* entry = GetLevelConfigEntry(gameWorld.world.levelsConfig, levelIndex);
    if (entry == nullptr) return;

    gameWorld.particles.Unload();
    gameWorld.player.smoking.emitterHandle = -1;
    UnloadRuntimeProps(gameWorld.world.props);
    UnloadLevel(gameWorld.world.level);
    gameWorld.world.currentLevelConfigIndex = levelIndex;
    gameWorld.world.currentLevelPath = entry->path;
    gameWorld.world.currentLevelConfigPath = entry->configPath;
    gameWorld.world.runtimeConfig = LoadLevelRuntimeConfig(gameWorld.world.currentLevelConfigPath);
    gameWorld.debugUi.configDirty = false;
    gameWorld.world.level = LoadLevel(
        gameWorld.world.currentLevelPath, gameWorld.world.runtimeConfig.skyboxPath);
    LoadRuntimeProps(gameWorld.world.props, gameWorld.world.runtimeConfig.props, gameWorld.render.fogShader);
    ApplyRuntimeRenderConfig(gameWorld);
    gameWorld.player.config = LoadPersonConfig(
        Utils::ResolveProjectPath(PERSON_CONFIG_PATH), DefaultPersonConfig());
    gameWorld.player.state = CreatePersonState(gameWorld.player.config);
    DestroyEntityRegistry(gameWorld.npcs);
    gameWorld.npcs = LoadEntityRegistry(gameWorld.world.runtimeConfig.characters);
    ApplyRuntimeRenderConfig(gameWorld);
    gameWorld.debugUi.selectedLevelNode = -1;
    gameWorld.debugUi.levelSidebarScroll = 0;
    gameWorld.debugUi.activeMenu = -1;
    ResetGameWorld(gameWorld);
}

void SaveLevelsConfigToDisk(const LevelsConfig& config) {
    SaveLevelsConfig(Utils::ResolveWritableProjectPath(LEVELS_CONFIG_PATH), config);
}

void MoveLevelConfigEntry(GameWorld& gameWorld, int fromIndex, int toIndex) {
    if (fromIndex < 0 || toIndex < 0) return;
    if (fromIndex >= static_cast<int>(gameWorld.world.levelsConfig.levels.size())) return;
    if (toIndex >= static_cast<int>(gameWorld.world.levelsConfig.levels.size())) return;

    std::swap(
        gameWorld.world.levelsConfig.levels[static_cast<std::size_t>(fromIndex)],
        gameWorld.world.levelsConfig.levels[static_cast<std::size_t>(toIndex)]);
    gameWorld.debugUi.selectedLevelConfigIndex = toIndex;
    gameWorld.debugUi.levelLoadScroll = Clamp(
        gameWorld.debugUi.levelLoadScroll,
        0,
        std::max(0, static_cast<int>(gameWorld.world.levelsConfig.levels.size()) - 1));
    if (gameWorld.world.currentLevelConfigIndex == fromIndex) {
        gameWorld.world.currentLevelConfigIndex = toIndex;
    } else if (gameWorld.world.currentLevelConfigIndex == toIndex) {
        gameWorld.world.currentLevelConfigIndex = fromIndex;
    }
    SaveLevelsConfigToDisk(gameWorld.world.levelsConfig);
}

void SaveCurrentLevelRuntimeConfig(GameWorld& gameWorld) {
    for (int i = 0;
         i < static_cast<int>(gameWorld.world.runtimeConfig.characters.size()) &&
         i < CharacterCount(gameWorld.npcs);
         ++i) {
        gameWorld.world.runtimeConfig.characters[i].position =
            CharacterAt(gameWorld.npcs, i).rootPosition;
        gameWorld.world.runtimeConfig.characters[i].rotationDegrees =
            EulerDegreesFromQuaternion(CharacterAt(gameWorld.npcs, i).rootRotation);
    }
    if (SaveLevelRuntimeConfig(
            gameWorld.world.currentLevelConfigPath, gameWorld.world.runtimeConfig)) {
        gameWorld.debugUi.configDirty = false;
    }
    SavePersonConfig(PERSON_CONFIG_PATH, gameWorld.player.config);
    SaveSmokingConfig(SMOKING_CONFIG_PATH, gameWorld.player.smokingConfig);
}

void ReloadCurrentLevelForConfig(GameWorld& gameWorld) {
    UnloadLevel(gameWorld.world.level);
    gameWorld.world.level = LoadLevel(
        gameWorld.world.currentLevelPath, gameWorld.world.runtimeConfig.skyboxPath);
    LoadRuntimeProps(gameWorld.world.props, gameWorld.world.runtimeConfig.props, gameWorld.render.fogShader);
    ApplyRuntimeRenderConfig(gameWorld);
    ResetGameWorld(gameWorld);
}

GameWorld LoadGameWorld() {
    GameWorld gameWorld = {};
    gameWorld.player.physics.reset(new physics_jolt::JoltWorld());
    gameWorld.player.physics->Init();

    ChaseCameraConfig fallbackChaseCamera = {3.5f, 1.0f, 25.0f, 50.0f};
    DebugCameraState fallbackDebugCamera = {false, 8.0f, 0.003f, 0.0f, 0.0f};

    gameWorld.world.statics = LoadStaticWorld();
    gameWorld.world.levelsConfig = LoadLevelsConfig(
        Utils::ResolveProjectPath(LEVELS_CONFIG_PATH), DefaultLevelsConfig());
    gameWorld.world.currentLevelConfigIndex = 0;
    const LevelConfigEntry* initialLevel = GetLevelConfigEntry(
        gameWorld.world.levelsConfig, gameWorld.world.currentLevelConfigIndex);
    gameWorld.world.currentLevelPath =
        initialLevel != nullptr ? initialLevel->path : std::string();
    gameWorld.world.currentLevelConfigPath =
        initialLevel != nullptr ? initialLevel->configPath : std::string();
    gameWorld.world.runtimeConfig =
        LoadLevelRuntimeConfig(gameWorld.world.currentLevelConfigPath);
    LoadFogShader(gameWorld.render.fogShader);
    LoadDebugIcons(gameWorld.render.debugIcons);
    gameWorld.world.level = LoadLevel(
        gameWorld.world.currentLevelPath, gameWorld.world.runtimeConfig.skyboxPath);
    LoadRuntimeProps(gameWorld.world.props, gameWorld.world.runtimeConfig.props, gameWorld.render.fogShader);
    ApplyRuntimeRenderConfig(gameWorld);
    gameWorld.player.config = LoadPersonConfig(
        Utils::ResolveProjectPath(PERSON_CONFIG_PATH), DefaultPersonConfig());
    gameWorld.player.smokingConfig = LoadSmokingConfig(
        Utils::ResolveProjectPath(SMOKING_CONFIG_PATH), DefaultSmokingConfig());
    gameWorld.player.state = CreatePersonState(gameWorld.player.config);
    gameWorld.player.input = LoadInputMap(
        Utils::ResolveProjectPath(INPUT_CONFIG_PATH), DefaultInputMap());
    gameWorld.npcs = LoadEntityRegistry(gameWorld.world.runtimeConfig.characters);
    ApplyRuntimeRenderConfig(gameWorld);
    gameWorld.render.chaseCamera = LoadChaseCameraConfig(
        Utils::ResolveProjectPath(CAMERA_CONFIG_PATH), fallbackChaseCamera);
    gameWorld.render.debugCamera = LoadDebugCameraStateConfig(
        Utils::ResolveProjectPath(CAMERA_CONFIG_PATH), fallbackDebugCamera);
    gameWorld.debugUi = DebugUiState{
        false, false, false, false, false, false, false,
        Vector2{20.0f, 50.0f},
        Vector2{660.0f, 50.0f},
        false, false, false, false, false,
        Vector2{20.0f, 190.0f},
        Vector2{20.0f, 330.0f},
        {"0", "0", "0"},
        {"0", "0", "0"},
        {"0", "0", "0"},
        {"0", "0", "0"},
        {"1", "1", "1"},
        0, 0, -1, 0, 0, -1, false,
        Vector2{0.0f, 0.0f},
        false, 0, 0, 0, 0, 0, -1, -1,
        Vector2{0.0f, 0.0f}
    };
    ResetGameWorld(gameWorld);
    DisableCursor();
    return gameWorld;
}

void UnloadGameWorld(GameWorld& gameWorld) {
    EnableCursor();
    gameWorld.particles.Unload();
    gameWorld.player.physics->Shutdown();
    DestroyEntityRegistry(gameWorld.npcs);
    UnloadRuntimeProps(gameWorld.world.props);
    UnloadLevel(gameWorld.world.level);
    UnloadFogShader(gameWorld.render.fogShader);
    UnloadDebugIcons(gameWorld.render.debugIcons);
    UnloadStaticWorld(gameWorld.world.statics);
}
