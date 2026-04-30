#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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
const float SPAWN_FRONT_UP_DEGREES = 25.0f;
const float CONTACT_RESTITUTION = 0.0f;
const float CONTACT_POSITION_PERCENT = 0.75f;
const float CONTACT_SLOP = 0.001f;

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
    ChaseCameraConfig chaseCamera;
    DebugCameraState debugCamera;
    float physicsAccumulator;
    float logElapsedSeconds;
    unsigned int logFrameIndex;
    std::vector<std::string> vehicleLogLines;
};

std::string FormatVector3(const Vector3& value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(4)
           << "(" << value.x << ", " << value.y << ", " << value.z << ")";
    return stream.str();
}

std::string FormatQuaternion(const Quaternion& value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(4)
           << "(" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << ")";
    return stream.str();
}

std::string BuildVehicleLogPath() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    char fileName[64] = {};
    if (localTime != nullptr) {
        std::strftime(fileName, sizeof(fileName), "vehicle_state_%Y%m%d_%H%M%S.log", localTime);
    } else {
        std::snprintf(fileName, sizeof(fileName), "vehicle_state.log");
    }

    return Utils::ResolveWritableProjectPath(fileName);
}

void AppendVehicleLogSnapshot(GameWorld& gameWorld) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(4);
    stream << "frame=" << gameWorld.logFrameIndex
           << " time=" << gameWorld.logElapsedSeconds
           << " grounded=" << (gameWorld.vehicle.grounded ? "true" : "false")
           << " drifting=" << (gameWorld.vehicle.drifting ? "true" : "false")
           << " speed_kph=" << gameWorld.vehicle.speedKph
           << " avg_front_steer_deg=" << GetAverageFrontSteerDegrees(gameWorld.vehicle)
           << " position=" << FormatVector3(gameWorld.vehicle.position)
           << " rotation=" << FormatQuaternion(gameWorld.vehicle.rotation)
           << " scale=" << FormatVector3(gameWorld.visual.scale)
           << " linear_velocity=" << FormatVector3(gameWorld.vehicle.linearVelocity)
           << " angular_velocity=" << FormatVector3(gameWorld.vehicle.angularVelocity)
           << " previous_linear_velocity=" << FormatVector3(gameWorld.vehicle.previousLinearVelocity)
           << " forward=" << FormatVector3(GetVehicleForward(gameWorld.vehicle))
           << " right=" << FormatVector3(GetVehicleRight(gameWorld.vehicle))
           << " up=" << FormatVector3(GetVehicleUp(gameWorld.vehicle))
           << " wheel_steer_deg=["
           << gameWorld.vehicle.wheelSteerAngles[0] * RAD2DEG << ", "
           << gameWorld.vehicle.wheelSteerAngles[1] * RAD2DEG << ", "
           << gameWorld.vehicle.wheelSteerAngles[2] * RAD2DEG << ", "
           << gameWorld.vehicle.wheelSteerAngles[3] * RAD2DEG << "]"
           << " wheel_world_pos=["
           << FormatVector3(GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, 0)) << ", "
           << FormatVector3(GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, 1)) << ", "
           << FormatVector3(GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, 2)) << ", "
           << FormatVector3(GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, 3)) << "]"
           << " wheel_engine_forces=["
           << FormatVector3(gameWorld.vehicle.wheelEngineForces[0]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelEngineForces[1]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelEngineForces[2]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelEngineForces[3]) << "]"
           << " wheel_grip_forces=["
           << FormatVector3(gameWorld.vehicle.wheelGripForces[0]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelGripForces[1]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelGripForces[2]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelGripForces[3]) << "]"
           << " wheel_drag_brake_forces=["
           << FormatVector3(gameWorld.vehicle.wheelDragBrakeForces[0]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelDragBrakeForces[1]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelDragBrakeForces[2]) << ", "
           << FormatVector3(gameWorld.vehicle.wheelDragBrakeForces[3]) << "]";
    gameWorld.vehicleLogLines.push_back(stream.str());
    ++gameWorld.logFrameIndex;
}

void FlushVehicleLog(const GameWorld& gameWorld) {
    std::string logPath = BuildVehicleLogPath();
    std::ofstream logFile(logPath.c_str(), std::ios::out | std::ios::trunc);
    if (!logFile.is_open()) {
        return;
    }

    logFile << "DragonRage vehicle state log\n";
    logFile << "samples=" << gameWorld.vehicleLogLines.size() << "\n";
    logFile << "collider_size=" << FormatVector3(gameWorld.vehicleConfig.colliderSize) << "\n";
    logFile << "center_of_mass=" << FormatVector3(gameWorld.vehicleConfig.centerOfMass) << "\n";
    logFile << "visual_scale=" << FormatVector3(gameWorld.visual.scale) << "\n";
    logFile << "wheel_offsets=["
            << FormatVector3(gameWorld.vehicleConfig.wheelOffsets[0]) << ", "
            << FormatVector3(gameWorld.vehicleConfig.wheelOffsets[1]) << ", "
            << FormatVector3(gameWorld.vehicleConfig.wheelOffsets[2]) << ", "
            << FormatVector3(gameWorld.vehicleConfig.wheelOffsets[3]) << "]\n";
    logFile << "\n";

    for (std::size_t i = 0; i < gameWorld.vehicleLogLines.size(); ++i) {
        logFile << gameWorld.vehicleLogLines[i] << "\n";
    }
}

physics::Transform3D BuildTransform(const Vector3& position, const Quaternion& rotation) {
    physics::Transform3D transform = physics::Transform3D::Identity();
    transform.position = position;
    transform.rotation = rotation;
    return transform;
}

Vector3 GetBoxInertia(const VehicleConfig& config) {
    float width = config.colliderSize.x;
    float height = config.colliderSize.y;
    float length = config.colliderSize.z;
    float factor = config.mass / 12.0f;
    return Vector3{
        factor * (height * height + length * length),
        factor * (width * width + length * length),
        factor * (width * width + height * height)
    };
}

Vector3 GetPointVelocity(const VehicleState& vehicle, const Vector3& point) {
    Vector3 offset = Vector3Subtract(point, vehicle.position);
    return Vector3Add(
        vehicle.linearVelocity,
        Vector3CrossProduct(vehicle.angularVelocity, offset));
}

float MoveTowardsScalar(float value, float target, float maxDelta) {
    if (fabsf(target - value) <= maxDelta) {
        return target;
    }
    return value + (target > value ? maxDelta : -maxDelta);
}

float SampleCurve(const std::vector<CurvePoint>& curve, float x) {
    if (curve.empty()) {
        return 0.0f;
    }
    if (x <= curve.front().x) {
        return curve.front().y;
    }
    for (std::size_t i = 1; i < curve.size(); ++i) {
        if (x <= curve[i].x) {
            float segment = curve[i].x - curve[i - 1].x;
            if (segment <= 0.0f) {
                return curve[i].y;
            }
            float t = (x - curve[i - 1].x) / segment;
            return Lerp(curve[i - 1].y, curve[i].y, t);
        }
    }
    return curve.back().y;
}

Vector3 ProjectDirectionOnPlane(const Vector3& direction, const Vector3& normal, const Vector3& fallback) {
    Vector3 projected = Vector3Subtract(
        direction,
        Vector3Scale(normal, Vector3DotProduct(direction, normal)));
    if (Vector3LengthSqr(projected) <= 0.0001f) {
        return fallback;
    }
    return Vector3Normalize(projected);
}

Vector3 GetWheelForwardForSteer(const VehicleState& vehicle, int wheelIndex) {
    Quaternion steerRotation = QuaternionFromAxisAngle(
        Vector3{0.0f, 1.0f, 0.0f},
        vehicle.wheelSteerAngles[wheelIndex]);
    Vector3 localForward = Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, steerRotation);
    return Vector3Normalize(Vector3RotateByQuaternion(localForward, vehicle.rotation));
}

Vector3 GetWheelRightForSteer(const VehicleState& vehicle, int wheelIndex) {
    Quaternion steerRotation = QuaternionFromAxisAngle(
        Vector3{0.0f, 1.0f, 0.0f},
        vehicle.wheelSteerAngles[wheelIndex]);
    Vector3 localRight = Vector3RotateByQuaternion(Vector3{1.0f, 0.0f, 0.0f}, steerRotation);
    return Vector3Normalize(Vector3RotateByQuaternion(localRight, vehicle.rotation));
}

void ApplyForceAtPoint(
    VehicleState& vehicle,
    const VehicleConfig& config,
    const Vector3& force,
    const Vector3& worldPoint,
    float deltaTime) {
    Vector3 offset = Vector3Subtract(worldPoint, vehicle.position);
    Vector3 torque = Vector3CrossProduct(offset, force);
    Vector3 inertia = GetBoxInertia(config);

    vehicle.linearVelocity = Vector3Add(
        vehicle.linearVelocity,
        Vector3Scale(force, deltaTime / config.mass));
    vehicle.angularVelocity = Vector3Add(
        vehicle.angularVelocity,
        Vector3Scale(
            Vector3{
                torque.x / inertia.x,
                torque.y / inertia.y,
                torque.z / inertia.z
            },
            deltaTime));
}

void IntegrateVehicleTransform(VehicleState& vehicle, float deltaTime) {
    vehicle.position = Vector3Add(
        vehicle.position,
        Vector3Scale(vehicle.linearVelocity, deltaTime));

    float angularSpeed = Vector3Length(vehicle.angularVelocity);
    if (angularSpeed <= 0.0001f) {
        return;
    }

    Quaternion deltaRotation = QuaternionFromAxisAngle(
        Vector3Scale(vehicle.angularVelocity, 1.0f / angularSpeed),
        angularSpeed * deltaTime);
    vehicle.rotation = QuaternionNormalize(QuaternionMultiply(deltaRotation, vehicle.rotation));
}

int ApplySuspensionRays(GameWorld& gameWorld, float deltaTime) {
    float groundTopY = gameWorld.world.groundY + gameWorld.world.roadThickness * 0.5f;
    int hits = 0;

    gameWorld.vehicle.wheelEngineForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};

    for (int i = 0; i < static_cast<int>(gameWorld.vehicleConfig.wheelOffsets.size()); ++i) {
        Vector3 rayOrigin = GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, i);
        float hitDistance = rayOrigin.y - groundTopY;
        if (hitDistance < 0.0f || hitDistance > gameWorld.vehicleConfig.suspensionRayLength) {
            continue;
        }

        ++hits;
        float springDistance = gameWorld.vehicleConfig.suspensionRestLength - hitDistance;
        if (springDistance <= 0.0f) {
            continue;
        }

        Vector3 pointVelocity = GetPointVelocity(gameWorld.vehicle, rayOrigin);
        float relativeVelocity = Vector3DotProduct(Vector3{0.0f, 1.0f, 0.0f}, pointVelocity);
        float forceMagnitude =
            springDistance * gameWorld.vehicleConfig.suspensionStrength -
            relativeVelocity * gameWorld.vehicleConfig.suspensionDamping;
        if (forceMagnitude <= 0.0f) {
            continue;
        }

        Vector3 force = Vector3{0.0f, forceMagnitude, 0.0f};
        gameWorld.vehicle.wheelEngineForces[i] = force;
        ApplyForceAtPoint(
            gameWorld.vehicle,
            gameWorld.vehicleConfig,
            force,
            rayOrigin,
            deltaTime);
    }

    return hits;
}

void ApplyDriveAndSteering(
    GameWorld& gameWorld,
    const VehicleInput& input,
    float deltaTime,
    bool suspensionContact) {
    VehicleState& vehicle = gameWorld.vehicle;
    const VehicleConfig& config = gameWorld.vehicleConfig;

    vehicle.wheelGripForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    vehicle.wheelDragBrakeForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    vehicle.speedKph = Vector3DotProduct(GetVehicleForward(vehicle), vehicle.linearVelocity) * 3.6f;

    float carMassShare = config.mass / static_cast<float>(config.wheelOffsets.size());
    for (int i = 0; i < static_cast<int>(config.wheelOffsets.size()); ++i) {
        bool isFrontWheel = i < 2;
        bool isPoweredWheel = i >= 2;
        float steerInput = input.steering * config.tireTurnSpeed;

        if (isFrontWheel) {
            float steerRatio = SampleCurve(config.maxTurnCurve, fabsf(vehicle.speedKph));
            float maxTurnRadians = DEG2RAD * config.tireMaxTurnDegrees * steerRatio;
            if (fabsf(steerInput) > 0.0f) {
                vehicle.wheelSteerAngles[i] = Clamp(
                    vehicle.wheelSteerAngles[i] + steerInput * deltaTime,
                    -maxTurnRadians,
                    maxTurnRadians);
            } else {
                vehicle.wheelSteerAngles[i] = MoveTowardsScalar(
                    vehicle.wheelSteerAngles[i],
                    0.0f,
                    config.tireTurnSpeed * deltaTime);
            }
        } else {
            vehicle.wheelSteerAngles[i] = 0.0f;
        }

        if (!suspensionContact) {
            continue;
        }

        Vector3 wheelCenter = GetWheelWorldPosition(vehicle, config, i);
        Vector3 wheelForward = ProjectDirectionOnPlane(
            GetWheelForwardForSteer(vehicle, i),
            Vector3{0.0f, 1.0f, 0.0f},
            Vector3{0.0f, 0.0f, -1.0f});
        Vector3 wheelRight = ProjectDirectionOnPlane(
            GetWheelRightForSteer(vehicle, i),
            Vector3{0.0f, 1.0f, 0.0f},
            Vector3{1.0f, 0.0f, 0.0f});
        Vector3 pointVelocity = GetPointVelocity(vehicle, wheelCenter);
        float forwardVelocity = Vector3DotProduct(wheelForward, pointVelocity);
        float sidewaysVelocity = Vector3DotProduct(wheelRight, pointVelocity);

        if (isPoweredWheel && input.throttle > 0.0f) {
            Vector3 engineForce = Vector3Scale(
                wheelForward,
                input.throttle * config.mass * config.enginePower * 0.5f);
            vehicle.wheelDragBrakeForces[i] = engineForce;
            ApplyForceAtPoint(vehicle, config, engineForce, wheelCenter, deltaTime);
        }

        Vector3 gripForce = Vector3Scale(
            wheelRight,
            -sidewaysVelocity * carMassShare * config.gripPower);
        vehicle.wheelGripForces[i] = gripForce;
        ApplyForceAtPoint(vehicle, config, gripForce, wheelCenter, deltaTime);

        float drag = SampleCurve(config.dragCurve, fabsf(vehicle.speedKph));
        Vector3 dragForce = Vector3Scale(
            wheelForward,
            -forwardVelocity * carMassShare * drag * 0.02f);
        Vector3 brakeForce = Vector3Scale(
            wheelForward,
            -forwardVelocity * carMassShare * config.brakePower * input.brake * 0.05f);
        Vector3 dragBrakeForce = Vector3Add(dragForce, brakeForce);
        vehicle.wheelDragBrakeForces[i] = Vector3Add(
            vehicle.wheelDragBrakeForces[i],
            dragBrakeForce);
        ApplyForceAtPoint(vehicle, config, dragBrakeForce, wheelCenter, deltaTime);
    }
}

void ApplyGroundImpulseAtPoint(
    VehicleState& vehicle,
    const VehicleConfig& config,
    const Vector3& worldPoint,
    float groundY) {
    Vector3 normal = Vector3{0.0f, 1.0f, 0.0f};
    Vector3 offset = Vector3Subtract(worldPoint, vehicle.position);
    Vector3 pointVelocity = GetPointVelocity(vehicle, worldPoint);
    float normalVelocity = Vector3DotProduct(pointVelocity, normal);

    if (normalVelocity < 0.0f) {
        Vector3 inertia = GetBoxInertia(config);
        Vector3 angularFactor = Vector3CrossProduct(offset, normal);
        float inverseMass = 1.0f / config.mass;
        float inverseInertia =
            (angularFactor.x * angularFactor.x) / inertia.x +
            (angularFactor.y * angularFactor.y) / inertia.y +
            (angularFactor.z * angularFactor.z) / inertia.z;
        float impulseMagnitude =
            -(1.0f + CONTACT_RESTITUTION) * normalVelocity /
            (inverseMass + inverseInertia);
        Vector3 impulse = Vector3Scale(normal, impulseMagnitude);
        Vector3 torqueImpulse = Vector3CrossProduct(offset, impulse);

        vehicle.linearVelocity = Vector3Add(
            vehicle.linearVelocity,
            Vector3Scale(impulse, inverseMass));
        vehicle.angularVelocity = Vector3Add(
            vehicle.angularVelocity,
            Vector3{
                torqueImpulse.x / inertia.x,
                torqueImpulse.y / inertia.y,
                torqueImpulse.z / inertia.z
            });
    }

    float penetration = groundY - worldPoint.y;
    if (penetration > CONTACT_SLOP) {
        vehicle.position.y += (penetration - CONTACT_SLOP) * CONTACT_POSITION_PERCENT;
    }
}

int ResolveGroundContactsFromShapeVertices(GameWorld& gameWorld) {
    if (!gameWorld.vehicleShape) {
        return 0;
    }

    float groundTopY = gameWorld.world.groundY + gameWorld.world.roadThickness * 0.5f;
    std::vector<Vector3> localPoints = gameWorld.vehicleShape->GetLocalContactPoints();
    int contacts = 0;

    for (std::size_t i = 0; i < localPoints.size(); ++i) {
        Vector3 worldPoint = Vector3Add(
            gameWorld.vehicle.position,
            Vector3RotateByQuaternion(localPoints[i], gameWorld.vehicle.rotation));
        if (worldPoint.y <= groundTopY + CONTACT_SLOP) {
            ApplyGroundImpulseAtPoint(
                gameWorld.vehicle,
                gameWorld.vehicleConfig,
                worldPoint,
                groundTopY);
            ++contacts;
        }
    }

    return contacts;
}

void SyncBodyFromVehicleState(physics::RigidBody& body, const VehicleState& vehicle) {
    body.SetTransform(BuildTransform(vehicle.position, vehicle.rotation));
    body.SetLinearVelocity(vehicle.linearVelocity);
    body.SetAngularVelocity(vehicle.angularVelocity);
}

void SyncVehicleStateFromBody(VehicleState& vehicle, const physics::RigidBody& body) {
    vehicle.position = body.GetPosition();
    vehicle.rotation = body.GetTransform().rotation;
    vehicle.linearVelocity = body.GetLinearVelocity();
    vehicle.angularVelocity = body.GetAngularVelocity();
    vehicle.grounded = body.IsGrounded();
    vehicle.speedKph = Vector3DotProduct(GetVehicleForward(vehicle), vehicle.linearVelocity) * 3.6f;
}

void ConfigurePhysicsScene(GameWorld& gameWorld) {
    gameWorld.physicsWorld = physics::PhysicsWorld();
    gameWorld.physicsWorld.SetGravity(Vector3{0.0f, -9.81f, 0.0f});

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
    vehicleDesc.gravityScale = 1.0f;
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
    visual.scale = Vector3{1.0f, 1.0f, 1.0f};
}

void UnloadCarVisual(CarVisual& visual) {
    (void)visual;
}

void ResetGameWorld(GameWorld& gameWorld) {
    ResetVehicleState(gameWorld.vehicle, gameWorld.vehicleConfig);
    gameWorld.vehicle.rotation = QuaternionFromAxisAngle(
        Vector3{1.0f, 0.0f, 0.0f},
        SPAWN_FRONT_UP_DEGREES * DEG2RAD);
    if (gameWorld.vehicleBody != nullptr) {
        SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
        gameWorld.physicsWorld.Step(0.0f);
        SyncVehicleStateFromBody(gameWorld.vehicle, *gameWorld.vehicleBody);
    }
    gameWorld.debugCamera.enabled = false;
    gameWorld.physicsAccumulator = 0.0f;
    gameWorld.camera = CreateChaseCamera(
        gameWorld.vehicle.position,
        gameWorld.vehicle.rotation,
        gameWorld.chaseCamera);
    gameWorld.vehicleLogLines.push_back("---- reset ----");
    AppendVehicleLogSnapshot(gameWorld);
}

GameWorld LoadGameWorld() {
    GameWorld gameWorld = {};
    gameWorld.vehicleBody = nullptr;
    gameWorld.roadBody = nullptr;
    gameWorld.logElapsedSeconds = 0.0f;
    gameWorld.logFrameIndex = 0;

    ChaseCameraConfig fallbackChaseCamera = {
        3.5f,
        1.0f,
        25.0f,
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
    gameWorld.chaseCamera = LoadChaseCameraConfig(
        Utils::ResolveProjectPath(CAMERA_CONFIG_PATH),
        fallbackChaseCamera);
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
    FlushVehicleLog(gameWorld);
    UnloadCarVisual(gameWorld.visual);
    UnloadStaticWorld(gameWorld.world);
}

void UpdateGameplay(GameWorld& gameWorld, float frameDelta) {
    float physicsStep = gameWorld.vehicleConfig.fixedTimeStep;
    gameWorld.physicsAccumulator += frameDelta;
    gameWorld.logElapsedSeconds += frameDelta;

    VehicleInput input = ReadVehicleInput(!gameWorld.debugCamera.enabled);
    int steps = 0;
    while (gameWorld.physicsAccumulator >= physicsStep && steps < 8) {
        gameWorld.vehicle.previousLinearVelocity = gameWorld.vehicle.linearVelocity;
        gameWorld.vehicle.linearVelocity.y -= 9.81f * physicsStep;
        int suspensionHits = ApplySuspensionRays(gameWorld, physicsStep);
        ApplyDriveAndSteering(
            gameWorld,
            input,
            physicsStep,
            suspensionHits > 0);
        float angularDampFactor = 1.0f / (1.0f + gameWorld.vehicleConfig.angularDamp * physicsStep);
        gameWorld.vehicle.angularVelocity = Vector3Scale(
            gameWorld.vehicle.angularVelocity,
            angularDampFactor);
        IntegrateVehicleTransform(gameWorld.vehicle, physicsStep);
        int groundContacts = ResolveGroundContactsFromShapeVertices(gameWorld);
        gameWorld.vehicle.grounded = suspensionHits > 0 || groundContacts > 0;
        gameWorld.vehicle.speedKph =
            Vector3DotProduct(GetVehicleForward(gameWorld.vehicle), gameWorld.vehicle.linearVelocity) * 3.6f;
        if (gameWorld.vehicleBody != nullptr) {
            SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
        }
        gameWorld.physicsAccumulator -= physicsStep;
        ++steps;
    }

    if (gameWorld.debugCamera.enabled) {
        UpdateDebugCamera(gameWorld.debugCamera, gameWorld.camera);
    } else {
        ApplyChaseCamera(
            gameWorld.camera,
            gameWorld.vehicle.position,
            gameWorld.vehicle.rotation,
            gameWorld.chaseCamera,
            frameDelta);
    }

    AppendVehicleLogSnapshot(gameWorld);
}

void DrawVehicle(const GameWorld& gameWorld) {
    Vector3 rotationAxis = Vector3{0.0f, 1.0f, 0.0f};
    float rotationAngle = 0.0f;
    QuaternionToAxisAngle(gameWorld.vehicle.rotation, &rotationAxis, &rotationAngle);

    rlPushMatrix();
    rlTranslatef(
        gameWorld.vehicle.position.x,
        gameWorld.vehicle.position.y,
        gameWorld.vehicle.position.z);
    rlRotatef(
        rotationAngle * RAD2DEG,
        rotationAxis.x,
        rotationAxis.y,
        rotationAxis.z);
    DrawCubeV(Vector3Zero(), gameWorld.vehicleConfig.colliderSize, RED);
    DrawCubeWiresV(
        Vector3Zero(),
        gameWorld.vehicleConfig.colliderSize,
        BLACK);
    rlPopMatrix();

    float groundTopY = gameWorld.world.groundY + gameWorld.world.roadThickness * 0.5f;
    for (int i = 0; i < static_cast<int>(gameWorld.vehicleConfig.wheelOffsets.size()); ++i) {
        Vector3 rayOrigin = GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, i);
        Vector3 rayEnd = Vector3Add(
            rayOrigin,
            Vector3{0.0f, -gameWorld.vehicleConfig.suspensionRayLength, 0.0f});
        bool hit =
            rayOrigin.y >= groundTopY &&
            rayOrigin.y - groundTopY <= gameWorld.vehicleConfig.suspensionRayLength;
        DrawLine3D(rayOrigin, rayEnd, hit ? GREEN : RED);
        DrawSphere(rayOrigin, 0.06f, hit ? GREEN : RED);
        DrawForceArrow(rayOrigin, gameWorld.vehicle.wheelEngineForces[i], 0.002f, BLUE);
        DrawForceArrow(rayOrigin, gameWorld.vehicle.wheelGripForces[i], 0.01f, YELLOW);
        DrawForceArrow(rayOrigin, gameWorld.vehicle.wheelDragBrakeForces[i], 0.01f, ORANGE);
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
        gameWorld.debugCamera.enabled ? "Mode: DEBUG CAMERA" : "Mode: CHASE",
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
        TextFormat("Velocity Y: %.2f", gameWorld.vehicle.linearVelocity.y),
        20,
        205,
        20,
        BLACK);
    DrawText(
        TextFormat("Collider: %.2f x %.2f x %.2f",
            gameWorld.vehicleConfig.colliderSize.x,
            gameWorld.vehicleConfig.colliderSize.y,
            gameWorld.vehicleConfig.colliderSize.z),
        20,
        230,
        20,
        BLACK);
    DrawText(
        TextFormat("Position: (%.2f, %.2f, %.2f)",
            gameWorld.vehicle.position.x,
            gameWorld.vehicle.position.y,
            gameWorld.vehicle.position.z),
        20,
        255,
        20,
        BLACK);
    DrawText(
        gameWorld.vehicle.grounded ? "Grounded" : "Airborne",
        20,
        280,
        20,
        gameWorld.vehicle.grounded ? DARKGREEN : RED);
    DrawText("W/S/A/D drive | Blue=susp Yellow=grip Orange=drive/drag/brake", 20, 305, 20, BLACK);
    DrawFPS(20, 335);
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
                ApplyChaseCamera(
                    gameWorld.camera,
                    gameWorld.vehicle.position,
                    gameWorld.vehicle.rotation,
                    gameWorld.chaseCamera,
                    1.0f / 60.0f);
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
