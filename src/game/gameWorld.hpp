#ifndef GAME_GAME_WORLD_HPP
#define GAME_GAME_WORLD_HPP

#include <memory>
#include <string>
#include <vector>

#include "raylib.h"

#include "debug/cameraDebug.hpp"
#include "debug/debugIcons.hpp"
#include "entity/entityRegistry.hpp"
#include "gameState.hpp"
#include "input/inputMap.hpp"
#include "level/levelData.hpp"
#include "level/levelsConfig.hpp"
#include "level/levelRuntimeConfig.hpp"
#include "gameplay/props.hpp"
#include "gameplay/smoking/smokingConfig.hpp"
#include "gameplay/smoking/smokingState.hpp"
#include "particles/particleSystem.hpp"
#include "personController.hpp"
#include "physics/jolt/joltWorld.hpp"
#include "render/fogRenderer.hpp"
#include "staticWorld.hpp"

struct DebugUiState {
    bool enabled;
    bool freeCameraActive;
    bool showForces;
    bool showPersonStatus;
    bool showPhysicsPanel;
    bool pinPersonStatus;
    bool pinPhysicsPanel;
    Vector2 personStatusPos;
    Vector2 physicsPanelPos;
    bool debugTeleportOpen;
    bool gameTeleportOpen;
    bool levelSidebarOpen;
    bool levelLoadOpen;
    bool levelConfigOpen;
    Vector2 debugTeleportPos;
    Vector2 gameTeleportPos;
    std::string debugTeleportInput[3];
    std::string gameTeleportInput[3];
    std::string levelPositionInput[3];
    std::string levelRotationInput[3];
    std::string levelScaleInput[3];
    int levelSidebarTab;
    int levelSidebarScroll;
    int selectedLevelNode;
    int inspectorDetailTab;
    int transformGizmoMode;
    int transformGizmoAxis;
    bool draggingTransformGizmo;
    Vector2 transformGizmoLastMouse;
    bool configDirty;
    int selectedLevelConfigIndex;
    int levelLoadScroll;
    int levelConfigScroll;
    int levelConfigTab;
    int activeTextField;
    int activeMenu;
    int draggingPanel;
    Vector2 dragOffset;
    bool personPanelOpen;
    int  personPanelTab;
    int  personPanelScroll;
    std::string activeTextFieldBuffer;
    bool    smokingGizmoDragging  = false;
    int     smokingGizmoAxis      = -1;
    Vector2 smokingGizmoLastMouse = {};
};

// ─── Subsystems ───────────────────────────────────────────────────────────────

struct WorldContext {
    StaticWorld statics;
    std::vector<RuntimeProp> props;
    LevelsConfig levelsConfig;
    LevelData level;
    int currentLevelConfigIndex;
    std::string currentLevelPath;
    std::string currentLevelConfigPath;
    LevelRuntimeConfig runtimeConfig;
};

struct PlayerContext {
    PersonConfig config;
    PersonState state;
    InputMap input;
    SmokingConfig smokingConfig;
    SmokingState smoking;
    std::unique_ptr<physics_jolt::JoltWorld> physics;
    float physicsAccumulator;
};

struct RenderContext {
    FogShader fogShader;
    DebugIcons debugIcons;
    Camera camera;
    ChaseCameraConfig chaseCamera;
    DebugCameraState debugCamera;
};

// ─── GameWorld ────────────────────────────────────────────────────────────────

struct GameWorld {
    WorldContext world;
    PlayerContext player;
    EntityRegistry npcs;
    RenderContext render;
    DebugUiState debugUi;
    ParticleSystem particles;
};

#endif
