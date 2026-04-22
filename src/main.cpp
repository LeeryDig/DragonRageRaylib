#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <memory>
#include <string>

#include "debug/cameraDebug.hpp"
#include "gameState.hpp"
#include "physics/collisionLayers.hpp"
#include "physics/physicsMaterial.hpp"
#include "physics/physicsWorld.hpp"
#include "physics/shapes/boxShape.hpp"
#include "staticWorld.hpp"
#include "utils.hpp"
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
    physics::PhysicsWorld physicsWorld;
    physics::RigidBody* vehicleBody;
    physics::StaticBody* roadBody;
    std::shared_ptr<const physics::BoxShape> vehicleShape;
    std::shared_ptr<const physics::BoxShape> roadShape;
    CarVisual visual;
    Camera camera;
    CockpitCameraConfig cockpitCamera;
    DebugCameraState debugCamera;
    float physicsAccumulator;
};

void RefreshWheelSupport(GameWorld& gameWorld);

physics::Transform3D BuildTransform(const Vector3& position, const Quaternion& rotation) {
    physics::Transform3D transform = physics::Transform3D::Identity();
    transform.position = position;
    transform.rotation = rotation;
    return transform;
}

void SyncBodyFromVehicleState(physics::RigidBody& body, const VehicleState& vehicle) {
    body.SetTransform(BuildTransform(vehicle.position, vehicle.rotation));
    body.SetLinearVelocity(vehicle.linearVelocity);
    body.SetAngularVelocity(vehicle.angularVelocity);
}

void SyncVehicleStateFromBody(VehicleState& vehicle, const physics::RigidBody& body) {
    vehicle.position = body.GetPosition();
    vehicle.linearVelocity = body.GetLinearVelocity();
    vehicle.angularVelocity = body.GetAngularVelocity();
    vehicle.speedKph = Vector3DotProduct(GetVehicleForward(vehicle), vehicle.linearVelocity) * 3.6f;
}

void RefreshWheelSupport(GameWorld& gameWorld) {
    Vector3 roadCenter = GetRoadColliderCenter(gameWorld.world);
    Vector3 roadSize = GetRoadColliderSize(gameWorld.world);
    float roadTopY = roadCenter.y + roadSize.y * 0.5f;
    float roadMinX = roadCenter.x - roadSize.x * 0.5f;
    float roadMaxX = roadCenter.x + roadSize.x * 0.5f;
    float roadMinZ = roadCenter.z - roadSize.z * 0.5f;
    float roadMaxZ = roadCenter.z + roadSize.z * 0.5f;
    const float wheelReach = 0.45f;
    const float wheelBelowTolerance = 0.08f;

    gameWorld.vehicle.supportedWheelCount = 0;
    for (int i = 0; i < 4; ++i) {
        Vector3 wheelPosition = GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, i);
        bool withinRoadX = wheelPosition.x >= roadMinX && wheelPosition.x <= roadMaxX;
        bool withinRoadZ = wheelPosition.z >= roadMinZ && wheelPosition.z <= roadMaxZ;
        bool withinRoadY =
            wheelPosition.y >= roadTopY - wheelBelowTolerance &&
            wheelPosition.y <= roadTopY + wheelReach;
        bool supported = withinRoadX && withinRoadZ && withinRoadY;
        gameWorld.vehicle.wheelSupported[i] = supported;
        if (supported) {
            ++gameWorld.vehicle.supportedWheelCount;
        }
    }

    gameWorld.vehicle.grounded = gameWorld.vehicle.supportedWheelCount > 0;
}

void ConfigurePhysicsScene(GameWorld& gameWorld) {
    gameWorld.physicsWorld = physics::PhysicsWorld();
    gameWorld.physicsWorld.SetGravity(Vector3{0.0f, 0.0f, 0.0f});

    physics::StaticBodyDesc roadDesc;
    roadDesc.transform = BuildTransform(GetRoadColliderCenter(gameWorld.world), QuaternionIdentity());
    roadDesc.defaultLayer = physics::ToMask(physics::CollisionLayer::World);
    roadDesc.defaultMask = physics::ToMask(physics::CollisionLayer::Vehicle);
    gameWorld.roadBody = gameWorld.physicsWorld.CreateStaticBody(roadDesc);

    gameWorld.roadShape.reset(new physics::BoxShape(GetRoadColliderSize(gameWorld.world)));

    physics::ColliderDesc roadCollider;
    roadCollider.shape = gameWorld.roadShape;
    roadCollider.layer = physics::ToMask(physics::CollisionLayer::World);
    roadCollider.mask = physics::ToMask(physics::CollisionLayer::Vehicle);
    roadCollider.material.name = "road";
    roadCollider.material.friction = 1.0f;
    gameWorld.roadBody->AddCollider(roadCollider);

    physics::RigidBodyDesc vehicleDesc;
    vehicleDesc.transform = BuildTransform(gameWorld.vehicle.position, gameWorld.vehicle.rotation);
    vehicleDesc.defaultLayer = physics::ToMask(physics::CollisionLayer::Vehicle);
    vehicleDesc.defaultMask = physics::ToMask(physics::CollisionLayer::World);
    vehicleDesc.mass = gameWorld.vehicleConfig.mass;
    vehicleDesc.gravityScale = 0.0f;
    vehicleDesc.linearDamp = 0.0f;
    vehicleDesc.angularDamp = 0.0f;
    gameWorld.vehicleBody = gameWorld.physicsWorld.CreateRigidBody(vehicleDesc);

    gameWorld.vehicleShape.reset(new physics::BoxShape(gameWorld.vehicleConfig.colliderSize));

    physics::ColliderDesc vehicleCollider;
    vehicleCollider.shape = gameWorld.vehicleShape;
    vehicleCollider.layer = physics::ToMask(physics::CollisionLayer::Vehicle);
    vehicleCollider.mask = physics::ToMask(physics::CollisionLayer::World);
    vehicleCollider.material.name = "car";
    vehicleCollider.material.friction = 0.8f;
    gameWorld.vehicleBody->AddCollider(vehicleCollider);

    SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
    gameWorld.physicsWorld.Step(0.0f);
    SyncVehicleStateFromBody(gameWorld.vehicle, *gameWorld.vehicleBody);
    RefreshWheelSupport(gameWorld);
}

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
    std::string modelPath = Utils::ResolveProjectPath("resources/models/CockpitF1.glb");
    std::string texturePath = Utils::ResolveProjectPath("resources/textures/F1CockpitDiff.png");
    visual.model = LoadModel(modelPath.c_str());
    visual.texture = LoadTexture(texturePath.c_str());
    visual.scale = Vector3{1.0f, 1.0f, 1.0f};
    SetMaterialTexture(&visual.model.materials[0], MATERIAL_MAP_DIFFUSE, visual.texture);
}

void UnloadCarVisual(CarVisual& visual) {
    UnloadTexture(visual.texture);
    UnloadModel(visual.model);
}

void ResetGameWorld(GameWorld& gameWorld) {
    ResetVehicleState(gameWorld.vehicle, gameWorld.vehicleConfig);
    if (gameWorld.vehicleBody != nullptr) {
        SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
        gameWorld.physicsWorld.Step(0.0f);
        SyncVehicleStateFromBody(gameWorld.vehicle, *gameWorld.vehicleBody);
    }
    RefreshWheelSupport(gameWorld);
    gameWorld.debugCamera.enabled = false;
    gameWorld.physicsAccumulator = 0.0f;
    gameWorld.camera = CreateCockpitCamera(
        gameWorld.vehicle.position,
        gameWorld.vehicle.rotation,
        gameWorld.cockpitCamera);
}

GameWorld LoadGameWorld() {
    GameWorld gameWorld = {};
    gameWorld.vehicleBody = nullptr;
    gameWorld.roadBody = nullptr;

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
    gameWorld.vehicleConfig = LoadVehicleConfig(
        Utils::ResolveProjectPath(VEHICLE_CONFIG_PATH),
        DefaultVehicleConfig());
    gameWorld.vehicle = CreateVehicleState(gameWorld.vehicleConfig);
    gameWorld.cockpitCamera = LoadCockpitCameraConfig(
        Utils::ResolveProjectPath(CAMERA_CONFIG_PATH),
        fallbackCockpitCamera);
    gameWorld.debugCamera = LoadDebugCameraStateConfig(
        Utils::ResolveProjectPath(CAMERA_CONFIG_PATH),
        fallbackDebugCamera);
    ConfigurePhysicsScene(gameWorld);
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
        RefreshWheelSupport(gameWorld);
        StepVehiclePhysics(
            gameWorld.vehicle,
            gameWorld.vehicleConfig,
            input,
            physicsStep);
        if (gameWorld.vehicleBody != nullptr) {
            SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
            gameWorld.physicsWorld.Step(0.0f);
            SyncVehicleStateFromBody(gameWorld.vehicle, *gameWorld.vehicleBody);
        }
        RefreshWheelSupport(gameWorld);
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

    DrawCubeWiresV(
        gameWorld.vehicle.position,
        gameWorld.vehicleConfig.colliderSize,
        SKYBLUE);

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
    
    DrawText("F2 respawn", 20, 95, 20, BLACK);
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
        if (IsKeyPressed(KEY_F2)) {
            ResetGameWorld(gameWorld);
        }

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
