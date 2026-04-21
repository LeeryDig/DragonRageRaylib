#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <string>

#include "debug/cameraDebug.hpp"
#include "gameState.hpp"
#include "staticWorld.hpp"
#include "vehiclePhysics.hpp"

SysState sysState = SysState::PLAYING;

namespace {

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;
const char* CAMERA_CONFIG_PATH = "resources/config/camera.json";
const char* VEHICLE_CONFIG_PATH = "resources/config/vehicle.json";

struct CarVisual {
    Model model;
    Texture2D texture;
    Vector3 scale;
};

struct GameWorld {
    StaticWorld world;
    VehicleConfig vehicleConfig;
    VehicleState vehicle;
    WorldCollisionConfig collision;
    CarVisual visual;
    Camera camera;
    CockpitCameraConfig cockpitCamera;
    DebugCameraState debugCamera;
    float physicsAccumulator;
};

void DrawForceArrow(Vector3 origin, Vector3 force, float scale, Color color) {
    if (Vector3LengthSqr(force) <= 0.0001f) {
        return;
    }

    Vector3 scaledForce = Vector3Scale(force, scale);
    Vector3 end = Vector3Add(origin, scaledForce);
    Vector3 direction = Vector3Normalize(scaledForce);
    float length = Vector3Length(scaledForce);
    float headLength = std::min(0.35f, length * 0.35f);
    Vector3 shaftEnd = Vector3Subtract(end, Vector3Scale(direction, headLength));

    DrawLine3D(origin, shaftEnd, color);
    DrawCylinderEx(shaftEnd, end, 0.08f, 0.0f, 6, color);
}

VehicleInput ReadVehicleInput(bool controlsEnabled) {
    if (!controlsEnabled) {
        return VehicleInput{0.0f, 0.0f, 0.0f};
    }

    float throttle = IsKeyDown(KEY_W) ? 1.0f : 0.0f;
    float brake = IsKeyDown(KEY_S) ? 1.0f : 0.0f;
    float steering = 0.0f;
    if (IsKeyDown(KEY_A)) {
        steering += 1.0f;
    }
    if (IsKeyDown(KEY_D)) {
        steering -= 1.0f;
    }

    return VehicleInput{throttle, brake, steering};
}

void LoadCarVisual(CarVisual& visual) {
    visual.model = LoadModel("resources/models/CockpitF1.glb");
    visual.texture = LoadTexture("resources/textures/F1CockpitDiff.png");
    visual.scale = Vector3{1.0f, 1.0f, 1.0f};
    SetMaterialTexture(&visual.model.materials[0], MATERIAL_MAP_DIFFUSE, visual.texture);
}

void UnloadCarVisual(CarVisual& visual) {
    UnloadTexture(visual.texture);
    UnloadModel(visual.model);
}

void ResetGameWorld(GameWorld& gameWorld) {
    ResetVehicleState(gameWorld.vehicle, gameWorld.vehicleConfig);
    gameWorld.debugCamera.enabled = false;
    gameWorld.physicsAccumulator = 0.0f;
    gameWorld.camera = CreateCockpitCamera(
        gameWorld.vehicle.position,
        gameWorld.vehicle.rotation,
        gameWorld.cockpitCamera);
}

GameWorld LoadGameWorld() {
    GameWorld gameWorld = {};

    CockpitCameraConfig fallbackCockpitCamera = {
        Vector3{0.0f, 0.22f, 0.1f},
        Vector3{0.0f, 0.4f, -1.0f},
        50.0f
    };
    DebugCameraState fallbackDebugCamera = {
        false,
        8.0f,
        0.003f,
        0.0f,
        0.0f
    };

    gameWorld.world = LoadStaticWorld();
    gameWorld.vehicleConfig = LoadVehicleConfig(VEHICLE_CONFIG_PATH, DefaultVehicleConfig());
    gameWorld.vehicle = CreateVehicleState(gameWorld.vehicleConfig);
    gameWorld.collision = WorldCollisionConfig{
        gameWorld.world.groundY,
        gameWorld.world.roadHalfWidth
    };
    gameWorld.cockpitCamera = LoadCockpitCameraConfig(CAMERA_CONFIG_PATH, fallbackCockpitCamera);
    gameWorld.debugCamera = LoadDebugCameraStateConfig(CAMERA_CONFIG_PATH, fallbackDebugCamera);
    LoadCarVisual(gameWorld.visual);
    ResetGameWorld(gameWorld);

    return gameWorld;
}

void UnloadGameWorld(GameWorld& gameWorld) {
    EnableCursor();
    UnloadCarVisual(gameWorld.visual);
    UnloadStaticWorld(gameWorld.world);
}

void UpdateGameplay(GameWorld& gameWorld, float frameDelta) {
    float physicsStep = gameWorld.vehicleConfig.fixedTimeStep;
    gameWorld.physicsAccumulator += frameDelta;

    VehicleInput input = ReadVehicleInput(!gameWorld.debugCamera.enabled);
    int steps = 0;
    while (gameWorld.physicsAccumulator >= physicsStep && steps < 8) {
        StepVehiclePhysics(
            gameWorld.vehicle,
            gameWorld.vehicleConfig,
            input,
            gameWorld.collision,
            physicsStep);
        gameWorld.physicsAccumulator -= physicsStep;
        ++steps;
    }

    if (gameWorld.debugCamera.enabled) {
        UpdateDebugCamera(gameWorld.debugCamera, gameWorld.camera);
    } else {
        ApplyCockpitCamera(
            gameWorld.camera,
            gameWorld.vehicle.position,
            gameWorld.vehicle.rotation,
            gameWorld.cockpitCamera);
    }
}

void DrawVehicle(const GameWorld& gameWorld) {
    float yawDegrees = GetVehicleYawDegrees(gameWorld.vehicle);

    DrawModelEx(
        gameWorld.visual.model,
        gameWorld.vehicle.position,
        Vector3{0.0f, 1.0f, 0.0f},
        yawDegrees,
        gameWorld.visual.scale,
        WHITE);

    for (int i = 0; i < 4; ++i) {
        Vector3 wheelPosition = GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, i);
        DrawCubeV(
            wheelPosition,
            Vector3{0.2f, 0.2f, 0.2f},
            i < 2 ? RED : ORANGE);
        DrawForceArrow(wheelPosition, gameWorld.vehicle.wheelEngineForces[i], 0.015f, RED);
        DrawForceArrow(wheelPosition, gameWorld.vehicle.wheelGripForces[i], 0.02f, YELLOW);
        DrawForceArrow(wheelPosition, gameWorld.vehicle.wheelDragBrakeForces[i], 0.02f, ORANGE);
    }
}

void DrawGameplay(const GameWorld& gameWorld) {
    BeginMode3D(gameWorld.camera);
    DrawStaticWorld(gameWorld.world);
    DrawVehicle(gameWorld);
    EndMode3D();

    DrawText("W accelerate", 20, 20, 20, BLACK);
    DrawText("S brake", 20, 45, 20, BLACK);
    DrawText("A/D steer", 20, 70, 20, BLACK);
    DrawText("Space brake assist", 20, 95, 20, BLACK);
    DrawText("F1 debug camera", 20, 120, 20, BLACK);
    DrawText(
        gameWorld.debugCamera.enabled ? "Mode: DEBUG CAMERA" : "Mode: COCKPIT",
        20,
        155,
        20,
        gameWorld.debugCamera.enabled ? MAROON : DARKGREEN);
    DrawText(
        TextFormat("Speed: %d km/h", static_cast<int>(gameWorld.vehicle.speedKph)),
        20,
        180,
        20,
        BLACK);
    DrawText(
        TextFormat("Steer: %.2f deg", GetAverageFrontSteerDegrees(gameWorld.vehicle)),
        20,
        205,
        20,
        BLACK);
    DrawText(
        TextFormat("Position: (%.2f, %.2f, %.2f)",
            gameWorld.vehicle.position.x,
            gameWorld.vehicle.position.y,
            gameWorld.vehicle.position.z),
        20,
        230,
        20,
        BLACK);
    DrawText(
        gameWorld.vehicle.grounded ? "Grounded" : "Airborne",
        20,
        255,
        20,
        gameWorld.vehicle.grounded ? DARKGREEN : RED);
    DrawText("Red=engine Yellow=grip Orange=drag/brake", 20, 280, 20, BLACK);
    DrawFPS(20, 310);
}

}  // namespace

int main() {
    InitWindow(SCRWIDTH, SCRHEIGHT, "Dragon Rage");
    SetTargetFPS(60);

    GameWorld gameWorld = LoadGameWorld();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F1)) {
            gameWorld.debugCamera.enabled = !gameWorld.debugCamera.enabled;
            if (gameWorld.debugCamera.enabled) {
                DisableCursor();
                SyncDebugCameraRotation(gameWorld.debugCamera, gameWorld.camera);
            } else {
                EnableCursor();
                ApplyCockpitCamera(
                    gameWorld.camera,
                    gameWorld.vehicle.position,
                    gameWorld.vehicle.rotation,
                    gameWorld.cockpitCamera);
            }
        }

        UpdateGameplay(gameWorld, GetFrameTime());

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawGameplay(gameWorld);
        EndDrawing();
    }

    UnloadGameWorld(gameWorld);
    CloseWindow();

    return 0;
}
