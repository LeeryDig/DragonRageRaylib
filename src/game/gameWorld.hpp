#ifndef GAME_GAME_WORLD_HPP
#define GAME_GAME_WORLD_HPP

#include <memory>
#include <string>
#include <vector>

#include "raylib.h"

#include "debug/cameraDebug.hpp"
#include "gameState.hpp"
#include "interactionSystem.hpp"
#include "level/levelData.hpp"
#include "level/levelsConfig.hpp"
#include "level/levelRuntimeConfig.hpp"
#include "personController.hpp"
#include "physics/physicsWorld.hpp"
#include "render/fogRenderer.hpp"
#include "physics/jolt/joltWorld.hpp"
#include "physics/shapes/boxShape.hpp"
#include "staticWorld.hpp"
#include "vehiclePhysics.hpp"

struct CarVisual {
    Model model;
    Texture2D texture;
    Vector3 scale;
};

struct DebugUiState {
    bool enabled;
    bool freeCameraActive;
    bool showForces;
    bool showVehicleStatus;
    bool showVehiclePanel;
    bool showPhysicsPanel;
    bool pinVehicleStatus;
    bool pinVehiclePanel;
    bool pinPhysicsPanel;
    Vector2 vehicleStatusPos;
    Vector2 vehiclePanelPos;
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
    bool levelConfigDirty;
    int selectedLevelConfigIndex;
    int levelLoadScroll;
    int levelConfigScroll;
    int levelConfigTab;
    int activeTextField;
    int activeMenu;
    int draggingPanel;
    Vector2 dragOffset;
};

struct GameWorld {
    StaticWorld world;
    LevelsConfig levelsConfig;
    LevelData level;
    int currentLevelConfigIndex;
    std::string currentLevelPath;
    std::string currentLevelConfigPath;
    LevelRuntimeConfig currentLevelRuntimeConfig;
    FogShader fogShader;
    PersonConfig personConfig;
    PersonState person;
    InteractionSystem interactions;
    VehicleConfig vehicleConfig;
    VehicleState vehicle;
    physics::PhysicsWorld physicsWorld;
    std::unique_ptr<physics_jolt::JoltWorld> joltWorld;
    physics::RigidBody* vehicleBody;
    std::vector<physics::StaticBody*> levelBodies;
    std::vector<std::shared_ptr<const physics::Shape> > levelShapes;
    std::shared_ptr<const physics::BoxShape> vehicleShape;
    CarVisual visual;
    Camera camera;
    ChaseCameraConfig chaseCamera;
    DebugCameraState debugCamera;
    DebugUiState debugUi;
    float physicsAccumulator;
    float logElapsedSeconds;
    unsigned int logFrameIndex;
    std::vector<std::string> vehicleLogLines;
};

#endif
