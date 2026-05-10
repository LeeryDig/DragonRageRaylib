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
#include "level/levelLoader.hpp"
#include "staticWorld.hpp"
#include "utils.hpp"
#include "vehiclePhysics.hpp"

SysState sysState = SysState::PLAYING;

namespace {

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;
const char* CAMERA_CONFIG_PATH = "resources/config/camera.json";
const char* VEHICLE_CONFIG_PATH = "resources/config/vehicle.json";
const char* LEVEL_PATH = "resources/levels/levelteste.glb";
const float SPAWN_FRONT_UP_DEGREES = 25.0f;
const float CONTACT_RESTITUTION = 0.0f;
const float CONTACT_POSITION_PERCENT = 0.75f;
const float CONTACT_SLOP = 0.001f;
const float GRASS_HALF_WIDTH = 60.0f;
const float GRASS_EXTRA_LENGTH = 200.0f;

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
    int activeMenu;
    int draggingPanel;
    Vector2 dragOffset;
};

struct GameWorld {
    StaticWorld world;
    LevelData level;
    VehicleConfig vehicleConfig;
    VehicleState vehicle;
    physics::PhysicsWorld physicsWorld;
    physics::RigidBody* vehicleBody;
    std::vector<physics::StaticBody*> levelBodies;
    std::vector<std::shared_ptr<const physics::BoxShape> > levelShapes;
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

struct GroundBoxSample {
    Vector3 center;
    Vector3 size;
    Quaternion rotation;
};

bool TryRoadSurfaceRayHit(
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    float rayLength,
    const LevelRoadSurface& surface,
    float& hitDistance,
    Vector3& hitPoint,
    Vector3& hitNormal) {
    Ray ray = Ray{rayOrigin, rayDirection};
    bool hit = false;
    hitDistance = rayLength;

    if (!surface.vertices.empty() && surface.indices.size() >= 3) {
        for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3) {
            unsigned int ia = surface.indices[i];
            unsigned int ib = surface.indices[i + 1];
            unsigned int ic = surface.indices[i + 2];
            if (ia >= surface.vertices.size() || ib >= surface.vertices.size() || ic >= surface.vertices.size()) {
                continue;
            }
            RayCollision collision = GetRayCollisionTriangle(
                ray,
                surface.vertices[ia],
                surface.vertices[ib],
                surface.vertices[ic]);
            if (collision.hit && collision.distance >= 0.0f && collision.distance <= hitDistance) {
                hit = true;
                hitDistance = collision.distance;
                hitPoint = collision.point;
                Vector3 normal = collision.normal;
                if (Vector3LengthSqr(normal) <= 0.0001f) {
                    normal = Vector3Normalize(Vector3CrossProduct(
                        Vector3Subtract(surface.vertices[ib], surface.vertices[ia]),
                        Vector3Subtract(surface.vertices[ic], surface.vertices[ia])));
                }
                if (Vector3DotProduct(normal, Vector3{0.0f, 1.0f, 0.0f}) < 0.0f) {
                    normal = Vector3Negate(normal);
                }
                hitNormal = normal;
            }
        }
        return hit;
    }

    Quaternion inverseRotation = QuaternionInvert(surface.rotation);
    Vector3 localOrigin = Vector3RotateByQuaternion(Vector3Subtract(rayOrigin, surface.position), inverseRotation);
    Vector3 localDirection = Vector3RotateByQuaternion(rayDirection, inverseRotation);

    if (fabsf(localDirection.y) < 0.0001f) {
        return false;
    }

    float distance = -localOrigin.y / localDirection.y;
    if (distance < 0.0f || distance > rayLength) {
        return false;
    }

    Vector3 localHit = Vector3Add(localOrigin, Vector3Scale(localDirection, distance));
    if (fabsf(localHit.x) > surface.size.x * 0.5f || fabsf(localHit.z) > surface.size.z * 0.5f) {
        return false;
    }

    hitDistance = distance;
    hitPoint = Vector3Add(rayOrigin, Vector3Scale(rayDirection, distance));
    hitNormal = Vector3Normalize(Vector3RotateByQuaternion(Vector3{0.0f, 1.0f, 0.0f}, surface.rotation));
    if (Vector3DotProduct(hitNormal, Vector3{0.0f, 1.0f, 0.0f}) < 0.0f) {
        hitNormal = Vector3Negate(hitNormal);
    }
    return true;
}

bool TryRayObbHit(
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    float rayLength,
    const GroundBoxSample& box,
    float& hitDistance,
    Vector3& hitPoint) {
    Quaternion inverseRotation = QuaternionInvert(box.rotation);
    Vector3 localOrigin = Vector3RotateByQuaternion(Vector3Subtract(rayOrigin, box.center), inverseRotation);
    Vector3 localDirection = Vector3RotateByQuaternion(rayDirection, inverseRotation);
    Vector3 half = Vector3Scale(box.size, 0.5f);

    float tMin = 0.0f;
    float tMax = rayLength;
    float origins[3] = {localOrigin.x, localOrigin.y, localOrigin.z};
    float directions[3] = {localDirection.x, localDirection.y, localDirection.z};
    float mins[3] = {-half.x, -half.y, -half.z};
    float maxs[3] = {half.x, half.y, half.z};

    for (int axis = 0; axis < 3; ++axis) {
        if (fabsf(directions[axis]) < 0.0001f) {
            if (origins[axis] < mins[axis] || origins[axis] > maxs[axis]) {
                return false;
            }
            continue;
        }

        float invD = 1.0f / directions[axis];
        float t1 = (mins[axis] - origins[axis]) * invD;
        float t2 = (maxs[axis] - origins[axis]) * invD;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) {
            return false;
        }
    }

    hitDistance = tMin;
    hitPoint = Vector3Add(rayOrigin, Vector3Scale(rayDirection, hitDistance));
    return true;
}

bool IsPointInsideObb(const Vector3& point, const GroundBoxSample& box, float& topY) {
    Quaternion inverseRotation = QuaternionInvert(box.rotation);
    Vector3 localPoint = Vector3RotateByQuaternion(Vector3Subtract(point, box.center), inverseRotation);
    Vector3 half = Vector3Scale(box.size, 0.5f);
    if (fabsf(localPoint.x) > half.x || fabsf(localPoint.y) > half.y || fabsf(localPoint.z) > half.z) {
        return false;
    }

    Vector3 localTop = Vector3{localPoint.x, half.y, localPoint.z};
    Vector3 worldTop = Vector3Add(box.center, Vector3RotateByQuaternion(localTop, box.rotation));
    topY = worldTop.y;
    return true;
}

bool SampleRoadSurfaceAtPoint(const Vector3& point, const LevelRoadSurface& surface, Vector3& groundPoint, Vector3& groundNormal) {
    float hitDistance = 0.0f;
    Vector3 hitPoint = Vector3Zero();
    Vector3 hitNormal = Vector3{0.0f, 1.0f, 0.0f};
    Vector3 rayOrigin = Vector3Add(point, Vector3{0.0f, 1000.0f, 0.0f});
    if (TryRoadSurfaceRayHit(
            rayOrigin,
            Vector3{0.0f, -1.0f, 0.0f},
            2000.0f,
            surface,
            hitDistance,
            hitPoint,
            hitNormal)) {
        groundPoint = hitPoint;
        groundNormal = hitNormal;
        return true;
    }
    return false;
}

bool SampleGroundAtPoint(const GameWorld& gameWorld, const Vector3& point, Vector3& groundPoint, Vector3& groundNormal) {
    bool hit = false;
    float bestY = -INFINITY;

    for (std::size_t i = 0; i < gameWorld.level.roadSurfaces.size(); ++i) {
        Vector3 candidatePoint = Vector3Zero();
        Vector3 candidateNormal = Vector3{0.0f, 1.0f, 0.0f};
        if (SampleRoadSurfaceAtPoint(point, gameWorld.level.roadSurfaces[i], candidatePoint, candidateNormal) && candidatePoint.y > bestY) {
            hit = true;
            bestY = candidatePoint.y;
            groundPoint = candidatePoint;
            groundNormal = candidateNormal;
        }
    }

    for (std::size_t i = 0; i < gameWorld.level.colliders.size(); ++i) {
        const LevelBoxVolume& collider = gameWorld.level.colliders[i];
        GroundBoxSample box = GroundBoxSample{collider.position, collider.size, collider.rotation};
        float candidateY = 0.0f;
        if (IsPointInsideObb(point, box, candidateY) && candidateY > bestY) {
            hit = true;
            bestY = candidateY;
            groundPoint = Vector3{point.x, candidateY, point.z};
            groundNormal = Vector3{0.0f, 1.0f, 0.0f};
        }
    }

    return hit;
}

bool TryGroundRayHit(
    const GameWorld& gameWorld,
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    float rayLength,
    float& hitDistance,
    Vector3& hitPoint,
    Vector3* hitNormal = nullptr) {
    bool hit = false;
    hitDistance = rayLength;
    for (std::size_t i = 0; i < gameWorld.level.roadSurfaces.size(); ++i) {
        float candidateDistance = 0.0f;
        Vector3 candidatePoint = Vector3Zero();
        Vector3 candidateNormal = Vector3{0.0f, 1.0f, 0.0f};
        if (TryRoadSurfaceRayHit(
                rayOrigin,
                rayDirection,
                rayLength,
                gameWorld.level.roadSurfaces[i],
                candidateDistance,
                candidatePoint,
                candidateNormal) &&
            candidateDistance < hitDistance) {
            hit = true;
            hitDistance = candidateDistance;
            hitPoint = candidatePoint;
            if (hitNormal != nullptr) {
                *hitNormal = candidateNormal;
            }
        }
    }

    for (std::size_t i = 0; i < gameWorld.level.colliders.size(); ++i) {
        const LevelBoxVolume& collider = gameWorld.level.colliders[i];
        GroundBoxSample box = GroundBoxSample{collider.position, collider.size, collider.rotation};
        float candidateDistance = 0.0f;
        Vector3 candidatePoint = Vector3Zero();
        if (TryRayObbHit(rayOrigin, rayDirection, rayLength, box, candidateDistance, candidatePoint) &&
            candidateDistance < hitDistance) {
            hit = true;
            hitDistance = candidateDistance;
            hitPoint = candidatePoint;
            if (hitNormal != nullptr) {
                *hitNormal = Vector3{0.0f, 1.0f, 0.0f};
            }
        }
    }
    return hit;
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
    int hits = 0;

    gameWorld.vehicle.wheelEngineForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};

    for (int i = 0; i < static_cast<int>(gameWorld.vehicleConfig.wheelOffsets.size()); ++i) {
        Vector3 rayOrigin = GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, i);
        Vector3 suspensionUp = GetVehicleUp(gameWorld.vehicle);
        Vector3 rayDirection = Vector3Negate(suspensionUp);
        float hitDistance = 0.0f;
        Vector3 hitPoint = Vector3Zero();
        if (!TryGroundRayHit(
                gameWorld,
                rayOrigin,
                rayDirection,
                gameWorld.vehicleConfig.suspensionRayLength,
                hitDistance,
                hitPoint)) {
            continue;
        }

        ++hits;
        float springDistance = gameWorld.vehicleConfig.suspensionRestLength - hitDistance;
        if (springDistance <= 0.0f) {
            continue;
        }

        Vector3 pointVelocity = GetPointVelocity(gameWorld.vehicle, rayOrigin);
        float relativeVelocity = Vector3DotProduct(suspensionUp, pointVelocity);
        float forceMagnitude =
            springDistance * gameWorld.vehicleConfig.suspensionStrength -
            relativeVelocity * gameWorld.vehicleConfig.suspensionDamping;
        if (forceMagnitude <= 0.0f) {
            continue;
        }

        Vector3 force = Vector3Scale(suspensionUp, forceMagnitude);
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
            -forwardVelocity * carMassShare * config.brakePower * input.brake * config.brakeForceMultiplier);
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
    const Vector3& groundPoint,
    const Vector3& groundNormal) {
    Vector3 normal = Vector3Normalize(groundNormal);
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

    float penetration = Vector3DotProduct(Vector3Subtract(groundPoint, worldPoint), normal);
    if (penetration > CONTACT_SLOP) {
        vehicle.position = Vector3Add(
            vehicle.position,
            Vector3Scale(normal, (penetration - CONTACT_SLOP) * CONTACT_POSITION_PERCENT));
    }
}

int ResolveGroundContactsFromShapeVertices(GameWorld& gameWorld) {
    if (!gameWorld.vehicleShape) {
        return 0;
    }

    std::vector<Vector3> localPoints = gameWorld.vehicleShape->GetLocalContactPoints();
    int contacts = 0;

    for (std::size_t i = 0; i < localPoints.size(); ++i) {
        Vector3 worldPoint = Vector3Add(
            gameWorld.vehicle.position,
            Vector3RotateByQuaternion(localPoints[i], gameWorld.vehicle.rotation));
        Vector3 groundPoint = Vector3Zero();
        Vector3 groundNormal = Vector3{0.0f, 1.0f, 0.0f};
        if (!SampleGroundAtPoint(gameWorld, worldPoint, groundPoint, groundNormal)) {
            continue;
        }
        float penetration = Vector3DotProduct(Vector3Subtract(groundPoint, worldPoint), Vector3Normalize(groundNormal));
        if (penetration >= -CONTACT_SLOP) {
            ApplyGroundImpulseAtPoint(
                gameWorld.vehicle,
                gameWorld.vehicleConfig,
                worldPoint,
                groundPoint,
                groundNormal);
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

    gameWorld.levelBodies.clear();
    gameWorld.levelShapes.clear();
    gameWorld.levelBodies.reserve(gameWorld.level.colliders.size());
    gameWorld.levelShapes.reserve(gameWorld.level.colliders.size());

    for (std::size_t i = 0; i < gameWorld.level.colliders.size(); ++i) {
        const LevelBoxVolume& levelCollider = gameWorld.level.colliders[i];
        physics::StaticBodyDesc staticDesc;
        staticDesc.transform = BuildTransform(levelCollider.position, levelCollider.rotation);
        staticDesc.defaultLayer = physics::ToMask(physics::CollisionLayer::World);
        staticDesc.defaultMask = physics::ToMask(physics::CollisionLayer::Vehicle);
        physics::StaticBody* staticBody = gameWorld.physicsWorld.CreateStaticBody(staticDesc);

        std::shared_ptr<const physics::BoxShape> shape(new physics::BoxShape(levelCollider.size));
        gameWorld.levelShapes.push_back(shape);

        physics::ColliderDesc colliderDesc;
        colliderDesc.shape = shape;
        colliderDesc.layer = physics::ToMask(physics::CollisionLayer::World);
        colliderDesc.mask = physics::ToMask(physics::CollisionLayer::Vehicle);
        colliderDesc.material.name = levelCollider.name;
        colliderDesc.material.friction = 1.0f;
        staticBody->AddCollider(colliderDesc);
        gameWorld.levelBodies.push_back(staticBody);
    }

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
    if (gameWorld.level.playerSpawn.valid) {
        gameWorld.vehicle.position = gameWorld.level.playerSpawn.position;
        gameWorld.vehicle.rotation = gameWorld.level.playerSpawn.rotation;
    } else {
        gameWorld.vehicle.rotation = QuaternionFromAxisAngle(
            Vector3{1.0f, 0.0f, 0.0f},
            SPAWN_FRONT_UP_DEGREES * DEG2RAD);
    }
    if (gameWorld.vehicleBody != nullptr) {
        SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
        gameWorld.physicsWorld.Step(0.0f);
        SyncVehicleStateFromBody(gameWorld.vehicle, *gameWorld.vehicleBody);
    }
    gameWorld.debugCamera.enabled = false;
    gameWorld.debugUi.freeCameraActive = false;
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
    gameWorld.level = LoadLevel(LEVEL_PATH);
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
    gameWorld.debugUi = DebugUiState{
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        Vector2{20.0f, 50.0f},
        Vector2{320.0f, 50.0f},
        Vector2{660.0f, 50.0f},
        -1,
        -1,
        Vector2{0.0f, 0.0f}
    };
    ConfigurePhysicsScene(gameWorld);
    LoadCarVisual(gameWorld.visual);
    ResetGameWorld(gameWorld);

    return gameWorld;
}

void UnloadGameWorld(GameWorld& gameWorld) {
    EnableCursor();
    FlushVehicleLog(gameWorld);
    UnloadCarVisual(gameWorld.visual);
    UnloadLevel(gameWorld.level);
    UnloadStaticWorld(gameWorld.world);
}

void UpdateGameplay(GameWorld& gameWorld, float frameDelta) {
    float physicsStep = gameWorld.vehicleConfig.fixedTimeStep;
    gameWorld.physicsAccumulator += frameDelta;
    gameWorld.logElapsedSeconds += frameDelta;

    VehicleInput input = ReadVehicleInput(!gameWorld.debugUi.enabled);
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

    if (gameWorld.debugUi.enabled) {
        bool wantsFreeCamera = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
        if (wantsFreeCamera && !gameWorld.debugUi.freeCameraActive) {
            gameWorld.debugUi.freeCameraActive = true;
            DisableCursor();
        } else if (!wantsFreeCamera && gameWorld.debugUi.freeCameraActive) {
            gameWorld.debugUi.freeCameraActive = false;
            EnableCursor();
        }

        if (gameWorld.debugUi.freeCameraActive) {
            UpdateDebugCamera(gameWorld.debugCamera, gameWorld.camera);
        }
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

void DrawLevelCollidersDebug(const LevelData& level) {
    for (std::size_t i = 0; i < level.roadSurfaces.size(); ++i) {
        const LevelRoadSurface& surface = level.roadSurfaces[i];
        Vector3 rotationAxis = Vector3{0.0f, 1.0f, 0.0f};
        float rotationAngle = 0.0f;
        QuaternionToAxisAngle(surface.rotation, &rotationAxis, &rotationAngle);
        if (!surface.vertices.empty() && surface.indices.size() >= 3) {
            for (std::size_t t = 0; t + 2 < surface.indices.size(); t += 3) {
                unsigned int ia = surface.indices[t];
                unsigned int ib = surface.indices[t + 1];
                unsigned int ic = surface.indices[t + 2];
                if (ia < surface.vertices.size() && ib < surface.vertices.size() && ic < surface.vertices.size()) {
                    DrawLine3D(surface.vertices[ia], surface.vertices[ib], GREEN);
                    DrawLine3D(surface.vertices[ib], surface.vertices[ic], GREEN);
                    DrawLine3D(surface.vertices[ic], surface.vertices[ia], GREEN);
                }
            }
        } else {
            Vector3 debugSize = Vector3{surface.size.x, 0.03f, surface.size.z};
            rlPushMatrix();
            rlTranslatef(surface.position.x, surface.position.y, surface.position.z);
            rlRotatef(rotationAngle * RAD2DEG, rotationAxis.x, rotationAxis.y, rotationAxis.z);
            DrawCubeWiresV(Vector3Zero(), debugSize, GREEN);
            rlPopMatrix();
        }
    }

    for (std::size_t i = 0; i < level.colliders.size(); ++i) {
        const LevelBoxVolume& collider = level.colliders[i];
        Vector3 rotationAxis = Vector3{0.0f, 1.0f, 0.0f};
        float rotationAngle = 0.0f;
        QuaternionToAxisAngle(collider.rotation, &rotationAxis, &rotationAngle);
        rlPushMatrix();
        rlTranslatef(collider.position.x, collider.position.y, collider.position.z);
        rlRotatef(rotationAngle * RAD2DEG, rotationAxis.x, rotationAxis.y, rotationAxis.z);
        DrawCubeWiresV(Vector3Zero(), collider.size, BLUE);
        rlPopMatrix();
    }
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

}

bool DebugMenuItem(Rectangle rect, const char* text, bool enabled = true, bool checked = false) {
    Vector2 mouse = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovered ? Color{70, 70, 78, 255} : Color{42, 42, 48, 245});
    DrawRectangleLinesEx(rect, 1.0f, Color{78, 78, 86, 255});
    if (checked) {
        DrawText("✓", static_cast<int>(rect.x + 8), static_cast<int>(rect.y + 5), 18, RAYWHITE);
    }
    DrawText(text, static_cast<int>(rect.x + 28), static_cast<int>(rect.y + 6), 18, enabled ? RAYWHITE : GRAY);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void RestartLevel(GameWorld& gameWorld) {
    UnloadLevel(gameWorld.level);
    gameWorld.level = LoadLevel(LEVEL_PATH);
    gameWorld.vehicleConfig = LoadVehicleConfig(
        Utils::ResolveProjectPath(VEHICLE_CONFIG_PATH),
        DefaultVehicleConfig());
    gameWorld.vehicle = CreateVehicleState(gameWorld.vehicleConfig);
    ConfigurePhysicsScene(gameWorld);
    ResetGameWorld(gameWorld);
}

bool DebugButton(Rectangle rect, const char* text) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovered ? Color{78, 78, 88, 255} : Color{56, 56, 64, 255});
    DrawRectangleLinesEx(rect, 1.0f, Color{95, 95, 105, 255});
    int textWidth = MeasureText(text, 16);
    DrawText(text, static_cast<int>(rect.x + rect.width * 0.5f - textWidth * 0.5f), static_cast<int>(rect.y + 6), 16, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool DebugFloatSlider(Rectangle rect, const char* label, float& value, float minValue, float maxValue) {
    Vector2 mouse = GetMousePosition();
    Rectangle bar = Rectangle{rect.x + 150.0f, rect.y + 8.0f, rect.width - 230.0f, 8.0f};
    bool changed = false;
    if (CheckCollisionPointRec(mouse, Rectangle{bar.x, rect.y, bar.width, rect.height}) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        float t = Clamp((mouse.x - bar.x) / bar.width, 0.0f, 1.0f);
        value = minValue + (maxValue - minValue) * t;
        changed = true;
    }

    float normalized = Clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
    DrawText(label, static_cast<int>(rect.x), static_cast<int>(rect.y + 2), 16, RAYWHITE);
    DrawRectangleRec(bar, Color{55, 55, 62, 255});
    DrawRectangleRec(Rectangle{bar.x, bar.y, bar.width * normalized, bar.height}, Color{90, 130, 210, 255});
    DrawCircle(static_cast<int>(bar.x + bar.width * normalized), static_cast<int>(bar.y + bar.height * 0.5f), 6.0f, RAYWHITE);
    DrawText(TextFormat("%.3f", value), static_cast<int>(rect.x + rect.width - 70.0f), static_cast<int>(rect.y + 2), 16, LIGHTGRAY);
    return changed;
}

void DrawVector3Value(Vector2 pos, const char* label, const Vector3& value) {
    DrawText(TextFormat("%s: %.2f %.2f %.2f", label, value.x, value.y, value.z), static_cast<int>(pos.x), static_cast<int>(pos.y), 16, LIGHTGRAY);
}

bool BeginDebugPanel(
    DebugUiState& ui,
    int panelId,
    Vector2& position,
    bool& pinned,
    const char* title,
    float width,
    float height) {
    Vector2 mouse = GetMousePosition();
    Rectangle panelRect = Rectangle{position.x, position.y, width, height};
    Rectangle titleRect = Rectangle{position.x, position.y, width, 28.0f};
    Rectangle pinRect = Rectangle{position.x + width - 30.0f, position.y + 4.0f, 22.0f, 20.0f};

    if (ui.enabled && CheckCollisionPointRec(mouse, pinRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        pinned = !pinned;
    }

    if (ui.enabled && CheckCollisionPointRec(mouse, titleRect) && !CheckCollisionPointRec(mouse, pinRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.draggingPanel = panelId;
        ui.dragOffset = Vector2Subtract(mouse, position);
    }
    if (ui.draggingPanel == panelId) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            position = Vector2Subtract(mouse, ui.dragOffset);
            position.x = Clamp(position.x, 0.0f, static_cast<float>(GetScreenWidth()) - width);
            position.y = Clamp(position.y, 0.0f, static_cast<float>(GetScreenHeight()) - 28.0f);
        } else {
            ui.draggingPanel = -1;
        }
    }

    DrawRectangleRec(panelRect, Color{28, 28, 32, 220});
    DrawRectangleRec(titleRect, Color{42, 42, 48, 245});
    DrawRectangleLinesEx(panelRect, 1.0f, Color{80, 80, 88, 255});
    DrawText(title, static_cast<int>(position.x + 12.0f), static_cast<int>(position.y + 6.0f), 18, RAYWHITE);
    DrawRectangleRec(pinRect, pinned ? Color{80, 120, 80, 255} : Color{65, 65, 72, 255});
    DrawRectangleLinesEx(pinRect, 1.0f, Color{95, 95, 105, 255});
    DrawText("P", static_cast<int>(pinRect.x + 6.0f), static_cast<int>(pinRect.y + 2.0f), 16, RAYWHITE);
    return true;
}

void DrawDebugPanels(GameWorld& gameWorld) {
    bool showVehicleStatus = gameWorld.debugUi.showVehicleStatus && (gameWorld.debugUi.enabled || gameWorld.debugUi.pinVehicleStatus);
    bool showVehiclePanel = gameWorld.debugUi.showVehiclePanel && (gameWorld.debugUi.enabled || gameWorld.debugUi.pinVehiclePanel);
    bool showPhysicsPanel = gameWorld.debugUi.showPhysicsPanel && (gameWorld.debugUi.enabled || gameWorld.debugUi.pinPhysicsPanel);

    if (showVehicleStatus) {
        Vector2& pos = gameWorld.debugUi.vehicleStatusPos;
        BeginDebugPanel(gameWorld.debugUi, 0, pos, gameWorld.debugUi.pinVehicleStatus, "Vehicle Status", 280.0f, 132.0f);
        DrawText(TextFormat("Speed: %d km/h", static_cast<int>(gameWorld.vehicle.speedKph)), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 44), 18, RAYWHITE);
        DrawText(gameWorld.vehicle.grounded ? "Grounded" : "Airborne", static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 68), 18, gameWorld.vehicle.grounded ? GREEN : RED);
        DrawText(TextFormat("Pos: %.1f %.1f %.1f", gameWorld.vehicle.position.x, gameWorld.vehicle.position.y, gameWorld.vehicle.position.z), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 92), 18, RAYWHITE);
    }

    if (showVehiclePanel) {
        Vector2& pos = gameWorld.debugUi.vehiclePanelPos;
        BeginDebugPanel(gameWorld.debugUi, 1, pos, gameWorld.debugUi.pinVehiclePanel, "Vehicle Tuning", 560.0f, 520.0f);
        float x = pos.x + 14.0f;
        float y = pos.y + 42.0f;
        float w = 532.0f;
        float row = 24.0f;

        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "fixedTimeStep", gameWorld.vehicleConfig.fixedTimeStep, 0.001f, 0.033f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "mass", gameWorld.vehicleConfig.mass, 100.0f, 2500.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "enginePower", gameWorld.vehicleConfig.enginePower, 0.0f, 200.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "downhillMultiplier", gameWorld.vehicleConfig.downhillMultiplier, 0.0f, 5.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "brakePower", gameWorld.vehicleConfig.brakePower, 0.0f, 100.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "brakeForceMultiplier", gameWorld.vehicleConfig.brakeForceMultiplier, 0.0f, 10.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "tireTurnSpeed", gameWorld.vehicleConfig.tireTurnSpeed, 0.0f, 10.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "tireMaxTurnDegrees", gameWorld.vehicleConfig.tireMaxTurnDegrees, 0.0f, 80.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "gripPower", gameWorld.vehicleConfig.gripPower, 0.0f, 100.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "gripFront", gameWorld.vehicleConfig.gripFront, 0.0f, 5.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "gripRear", gameWorld.vehicleConfig.gripRear, 0.0f, 5.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "gripDriftFront", gameWorld.vehicleConfig.gripDriftFront, 0.0f, 5.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "gripDriftRear", gameWorld.vehicleConfig.gripDriftRear, 0.0f, 5.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "airPitchTorque", gameWorld.vehicleConfig.airPitchTorque, 0.0f, 200.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "extraGravity", gameWorld.vehicleConfig.extraGravity, 0.0f, 50.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "wallPenaltyMultiplier", gameWorld.vehicleConfig.wallPenaltyMultiplier, 0.0f, 5.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "wallSpinDamping", gameWorld.vehicleConfig.wallSpinDamping, 0.0f, 20.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "groundLinearDamp", gameWorld.vehicleConfig.groundLinearDamp, 0.0f, 20.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "angularDamp", gameWorld.vehicleConfig.angularDamp, 0.0f, 20.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "suspRestLength", gameWorld.vehicleConfig.suspensionRestLength, 0.0f, 2.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "suspRayLength", gameWorld.vehicleConfig.suspensionRayLength, 0.0f, 4.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "suspStrength", gameWorld.vehicleConfig.suspensionStrength, 0.0f, 100000.0f); y += row;
        DebugFloatSlider(Rectangle{x, y, w, 22.0f}, "suspDamping", gameWorld.vehicleConfig.suspensionDamping, 0.0f, 20000.0f); y += row + 4.0f;

        DrawVector3Value(Vector2{x, y}, "colliderSize", gameWorld.vehicleConfig.colliderSize); y += 20.0f;
        DrawVector3Value(Vector2{x, y}, "centerOfMass", gameWorld.vehicleConfig.centerOfMass); y += 20.0f;
        DrawText("Vector3/wheels/curves editing coming next", static_cast<int>(x), static_cast<int>(y), 16, GRAY);

        if (DebugButton(Rectangle{pos.x + 380.0f, pos.y + 486.0f, 160.0f, 26.0f}, "Apply collider")) {
            ConfigurePhysicsScene(gameWorld);
            SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
        }
    }

    if (showPhysicsPanel) {
        Vector2& pos = gameWorld.debugUi.physicsPanelPos;
        BeginDebugPanel(gameWorld.debugUi, 2, pos, gameWorld.debugUi.pinPhysicsPanel, "Physics Panel", 320.0f, 110.0f);
        DrawText("Physics debug tools coming soon", static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 50), 18, LIGHTGRAY);
    }
}

void DrawDebugMenuBar(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), 34, Color{28, 28, 32, 235});
    DrawRectangleLines(0, 0, GetScreenWidth(), 34, Color{70, 70, 78, 255});

    const char* items[] = {"Game", "Debug", "Vehicle", "Physics"};
    int menuX[4] = {};
    int menuW[4] = {};
    int x = 10;
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < 4; ++i) {
        int width = MeasureText(items[i], 20) + 28;
        menuX[i] = x;
        menuW[i] = width;
        Rectangle rect = Rectangle{static_cast<float>(x), 5.0f, static_cast<float>(width), 24.0f};
        bool hovered = CheckCollisionPointRec(mouse, rect);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.activeMenu = gameWorld.debugUi.activeMenu == i ? -1 : i;
        }
        DrawRectangleRec(rect, hovered || gameWorld.debugUi.activeMenu == i ? Color{62, 62, 70, 255} : Color{42, 42, 48, 255});
        DrawText(items[i], x + 14, 8, 20, RAYWHITE);
        x += width + 6;
    }

    if (gameWorld.debugUi.activeMenu == 0) {
        float dx = static_cast<float>(menuX[0]);
        if (DebugMenuItem(Rectangle{dx, 38, 190, 30}, "Restart Level")) {
            RestartLevel(gameWorld);
            gameWorld.debugUi.activeMenu = -1;
        }
        if (DebugMenuItem(Rectangle{dx, 68, 190, 30}, "Reset Car")) {
            ResetGameWorld(gameWorld);
            gameWorld.debugUi.activeMenu = -1;
        }
        DebugMenuItem(Rectangle{dx, 98, 190, 30}, "Load Level", false);
    } else if (gameWorld.debugUi.activeMenu == 1) {
        float dx = static_cast<float>(menuX[1]);
        if (DebugMenuItem(Rectangle{dx, 38, 220, 30}, "Show Forces", true, gameWorld.debugUi.showForces)) {
            gameWorld.debugUi.showForces = !gameWorld.debugUi.showForces;
        }
        if (DebugMenuItem(Rectangle{dx, 68, 220, 30}, "Show Vehicle Status", true, gameWorld.debugUi.showVehicleStatus)) {
            gameWorld.debugUi.showVehicleStatus = !gameWorld.debugUi.showVehicleStatus;
        }
    } else if (gameWorld.debugUi.activeMenu == 2) {
        float dx = static_cast<float>(menuX[2]);
        if (DebugMenuItem(Rectangle{dx, 38, 220, 30}, "Vehicle Tuning", true, gameWorld.debugUi.showVehiclePanel)) {
            gameWorld.debugUi.showVehiclePanel = !gameWorld.debugUi.showVehiclePanel;
        }
    } else if (gameWorld.debugUi.activeMenu == 3) {
        float dx = static_cast<float>(menuX[3]);
        if (DebugMenuItem(Rectangle{dx, 38, 220, 30}, "Physics Panel", true, gameWorld.debugUi.showPhysicsPanel)) {
            gameWorld.debugUi.showPhysicsPanel = !gameWorld.debugUi.showPhysicsPanel;
        }
    }

    DrawText("F1 close menu | Hold RMB + WASD/Q/Z to fly", GetScreenWidth() - 420, 9, 16, LIGHTGRAY);
}

void DrawGameplay(GameWorld& gameWorld) {
    BeginMode3D(gameWorld.camera);
    DrawLevel(gameWorld.level);
    DrawVehicle(gameWorld);
    if (gameWorld.debugUi.showForces) {
        for (int i = 0; i < static_cast<int>(gameWorld.vehicleConfig.wheelOffsets.size()); ++i) {
            Vector3 rayOrigin = GetWheelWorldPosition(gameWorld.vehicle, gameWorld.vehicleConfig, i);
            Vector3 rayDirection = Vector3Negate(GetVehicleUp(gameWorld.vehicle));
            Vector3 rayEnd = Vector3Add(rayOrigin, Vector3Scale(rayDirection, gameWorld.vehicleConfig.suspensionRayLength));
            float hitDistance = 0.0f;
            Vector3 hitPoint = Vector3Zero();
            bool hit = TryGroundRayHit(gameWorld, rayOrigin, rayDirection, gameWorld.vehicleConfig.suspensionRayLength, hitDistance, hitPoint);
            DrawLine3D(rayOrigin, rayEnd, hit ? GREEN : RED);
            if (hit) DrawSphere(hitPoint, 0.05f, BLUE);
            DrawForceArrow(rayOrigin, gameWorld.vehicle.wheelEngineForces[i], 0.002f, BLUE);
            DrawForceArrow(rayOrigin, gameWorld.vehicle.wheelGripForces[i], 0.01f, YELLOW);
            DrawForceArrow(rayOrigin, gameWorld.vehicle.wheelDragBrakeForces[i], 0.01f, ORANGE);
        }
    }
    EndMode3D();
    DrawDebugMenuBar(gameWorld);
    DrawDebugPanels(gameWorld);
}

}  // namespace

int main() {
    InitWindow(SCRWIDTH, SCRHEIGHT, "Dragon Rage");
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    if (monitorWidth > 0 && monitorHeight > 0) {
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowPosition(0, 0);
    }
    SetTargetFPS(60);

    GameWorld gameWorld = LoadGameWorld();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F1)) {
            gameWorld.debugUi.enabled = !gameWorld.debugUi.enabled;
            if (gameWorld.debugUi.enabled) {
                gameWorld.debugUi.freeCameraActive = false;
                EnableCursor();
                SyncDebugCameraRotation(gameWorld.debugCamera, gameWorld.camera);
            } else {
                gameWorld.debugUi.freeCameraActive = false;
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
