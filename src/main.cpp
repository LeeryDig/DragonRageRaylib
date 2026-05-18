#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "debug/cameraDebug.hpp"
#include "debug/levelDebugDraw.hpp"
#include "debug/ui/debugUi.hpp"
#include "game/gameWorld.hpp"
#include "gameState.hpp"
#include "interactionSystem.hpp"
#include "physics/collisionLayers.hpp"
#include "physics/physicsMaterial.hpp"
#include "physics/physicsWorld.hpp"
#include "physics/jolt/joltWorld.hpp"
#include "physics/shapes/boxShape.hpp"
#include "physics/shapes/meshShape.hpp"
#include "level/levelLoader.hpp"
#include "level/levelsConfig.hpp"
#include "personController.hpp"
#include "personCollision.hpp"
#include "staticWorld.hpp"
#include "utils.hpp"
#include "vehiclePhysics.hpp"

SysState sysState = SysState::PLAYING;

namespace {

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;
const char* CAMERA_CONFIG_PATH = "resources/config/camera.json";
const char* PERSON_CONFIG_PATH = "resources/config/person.json";
const char* LEVELS_CONFIG_PATH = "resources/config/levels.json";
const float SPAWN_FRONT_UP_DEGREES = 25.0f;
const float CONTACT_RESTITUTION = 0.0f;
const float CONTACT_POSITION_PERCENT = 0.75f;
const float CONTACT_SLOP = 0.001f;
const float GRASS_HALF_WIDTH = 60.0f;
const float GRASS_EXTRA_LENGTH = 200.0f;

std::string DisplayNameFromPath(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    std::string filename = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = filename.find_last_of('.');
    return dot == std::string::npos ? filename : filename.substr(0, dot);
}

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

bool TryCollisionMeshRayHit(
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    float rayLength,
    const LevelCollisionMesh& mesh,
    float& hitDistance,
    Vector3& hitPoint,
    Vector3& hitNormal) {
    Ray ray = Ray{rayOrigin, rayDirection};
    bool hit = false;
    hitDistance = rayLength;

    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        unsigned int ia = mesh.indices[i];
        unsigned int ib = mesh.indices[i + 1];
        unsigned int ic = mesh.indices[i + 2];
        if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() || ic >= mesh.vertices.size()) {
            continue;
        }
        RayCollision collision = GetRayCollisionTriangle(
            ray,
            mesh.vertices[ia],
            mesh.vertices[ib],
            mesh.vertices[ic]);
        if (collision.hit && collision.distance >= 0.0f && collision.distance <= hitDistance) {
            hit = true;
            hitDistance = collision.distance;
            hitPoint = collision.point;
            Vector3 normal = collision.normal;
            if (Vector3LengthSqr(normal) <= 0.0001f) {
                normal = Vector3Normalize(Vector3CrossProduct(
                    Vector3Subtract(mesh.vertices[ib], mesh.vertices[ia]),
                    Vector3Subtract(mesh.vertices[ic], mesh.vertices[ia])));
            }
            if (Vector3DotProduct(normal, rayDirection) > 0.0f) {
                normal = Vector3Negate(normal);
            }
            hitNormal = normal;
        }
    }
    return hit;
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

bool SampleCollisionMeshAtPoint(const Vector3& point, const LevelCollisionMesh& mesh, Vector3& groundPoint, Vector3& groundNormal) {
    float hitDistance = 0.0f;
    Vector3 hitPoint = Vector3Zero();
    Vector3 hitNormal = Vector3{0.0f, 1.0f, 0.0f};
    Vector3 rayOrigin = Vector3Add(point, Vector3{0.0f, 1000.0f, 0.0f});
    if (TryCollisionMeshRayHit(
            rayOrigin,
            Vector3{0.0f, -1.0f, 0.0f},
            2000.0f,
            mesh,
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

    for (std::size_t i = 0; i < gameWorld.level.collisionMeshes.size(); ++i) {
        Vector3 candidatePoint = Vector3Zero();
        Vector3 candidateNormal = Vector3{0.0f, 1.0f, 0.0f};
        if (SampleCollisionMeshAtPoint(point, gameWorld.level.collisionMeshes[i], candidatePoint, candidateNormal) && candidatePoint.y > bestY) {
            hit = true;
            bestY = candidatePoint.y;
            groundPoint = candidatePoint;
            groundNormal = candidateNormal;
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
    for (std::size_t i = 0; i < gameWorld.level.collisionMeshes.size(); ++i) {
        float candidateDistance = 0.0f;
        Vector3 candidatePoint = Vector3Zero();
        Vector3 candidateNormal = Vector3{0.0f, 1.0f, 0.0f};
        if (TryCollisionMeshRayHit(
                rayOrigin,
                rayDirection,
                rayLength,
                gameWorld.level.collisionMeshes[i],
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
    gameWorld.levelBodies.reserve(gameWorld.level.colliders.size() + gameWorld.level.collisionMeshes.size());
    gameWorld.levelShapes.reserve(gameWorld.level.colliders.size() + gameWorld.level.collisionMeshes.size());

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

    for (std::size_t i = 0; i < gameWorld.level.collisionMeshes.size(); ++i) {
        const LevelCollisionMesh& levelMesh = gameWorld.level.collisionMeshes[i];
        if (levelMesh.vertices.empty() || levelMesh.indices.size() < 3) {
            continue;
        }

        physics::StaticBodyDesc staticDesc;
        staticDesc.transform = BuildTransform(Vector3{0.0f, 0.0f, 0.0f}, Quaternion{0.0f, 0.0f, 0.0f, 1.0f});
        staticDesc.defaultLayer = physics::ToMask(physics::CollisionLayer::World);
        staticDesc.defaultMask = physics::ToMask(physics::CollisionLayer::Vehicle);
        physics::StaticBody* staticBody = gameWorld.physicsWorld.CreateStaticBody(staticDesc);

        std::shared_ptr<const physics::MeshShape> shape(new physics::MeshShape(levelMesh.vertices, levelMesh.indices, levelMesh.name));
        gameWorld.levelShapes.push_back(shape);

        physics::ColliderDesc colliderDesc;
        colliderDesc.shape = shape;
        colliderDesc.layer = physics::ToMask(physics::CollisionLayer::World);
        colliderDesc.mask = physics::ToMask(physics::CollisionLayer::Vehicle);
        colliderDesc.material.name = levelMesh.name;
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

void ReloadJoltLevelPhysics(GameWorld& gameWorld) {
    if (!gameWorld.joltWorld || !gameWorld.joltWorld->IsReady()) {
        return;
    }
    gameWorld.joltWorld->LoadLevel(gameWorld.level);
    gameWorld.joltWorld->CreateCharacter(gameWorld.personConfig, gameWorld.person.position);
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
    Vector3 spawnPosition = Vector3{0.0f, 2.0f, 0.0f};
    float spawnYaw = 0.0f;
    if (gameWorld.level.playerSpawn.valid) {
        spawnPosition = gameWorld.level.playerSpawn.position;
        Vector3 spawnForward = Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, gameWorld.level.playerSpawn.rotation);
        spawnYaw = atan2f(spawnForward.x, -spawnForward.z);
    }

    ResetPersonState(gameWorld.person, gameWorld.personConfig, spawnPosition, spawnYaw);
    if (gameWorld.joltWorld && gameWorld.joltWorld->IsReady()) {
        gameWorld.joltWorld->LoadLevel(gameWorld.level);
        gameWorld.joltWorld->CreateCharacter(gameWorld.personConfig, gameWorld.person.position);
    }
    Vector3 groundPoint = Vector3Zero();
    Vector3 groundNormal = Vector3{0.0f, 1.0f, 0.0f};
    if ((!gameWorld.joltWorld || !gameWorld.joltWorld->IsReady()) && SampleGroundAtPoint(gameWorld, gameWorld.person.position, groundPoint, groundNormal)) {
        gameWorld.person.position.y = groundPoint.y;
        gameWorld.person.grounded = true;
    }

    gameWorld.debugCamera.enabled = false;
    gameWorld.debugUi.freeCameraActive = false;
    gameWorld.physicsAccumulator = 0.0f;
    gameWorld.camera = Camera{};
    ApplyPersonCamera(gameWorld.camera, gameWorld.person, gameWorld.personConfig, 1.0f);
    if (gameWorld.interactions.dialogueOpen) {
        gameWorld.interactions.dialogueOpen = false;
        gameWorld.interactions.activeDialogueIndex = -1;
    }
    sysState = SysState::PLAYING;
}

GameWorld LoadGameWorld() {
    GameWorld gameWorld = {};
    gameWorld.joltWorld.reset(new physics_jolt::JoltWorld());
    gameWorld.joltWorld->Init();
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
    gameWorld.levelsConfig = LoadLevelsConfig(
        Utils::ResolveProjectPath(LEVELS_CONFIG_PATH),
        DefaultLevelsConfig());
    gameWorld.currentLevelConfigIndex = 0;
    const LevelConfigEntry* initialLevel = GetLevelConfigEntry(gameWorld.levelsConfig, gameWorld.currentLevelConfigIndex);
    gameWorld.currentLevelPath = initialLevel != nullptr ? initialLevel->path : std::string();
    gameWorld.currentLevelConfigPath = initialLevel != nullptr ? initialLevel->configPath : std::string();
    gameWorld.currentLevelRuntimeConfig = LoadLevelRuntimeConfig(gameWorld.currentLevelConfigPath);
    gameWorld.level = LoadLevel(gameWorld.currentLevelPath, gameWorld.currentLevelRuntimeConfig.skyboxPath);
    gameWorld.personConfig = LoadPersonConfig(
        Utils::ResolveProjectPath(PERSON_CONFIG_PATH),
        DefaultPersonConfig());
    gameWorld.person = CreatePersonState(gameWorld.personConfig);
    gameWorld.interactions = LoadInteractionSystem(gameWorld.currentLevelRuntimeConfig.characters);
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
        false,
        false,
        false,
        false,
        false,
        Vector2{20.0f, 190.0f},
        Vector2{20.0f, 330.0f},
        {"0", "0", "0"},
        {"0", "0", "0"},
        {"0", "0", "0"},
        {"0", "0", "0"},
        {"1", "1", "1"},
        0,
        0,
        -1,
        0,
        0,
        -1,
        false,
        Vector2{0.0f, 0.0f},
        false,
        0,
        0,
        0,
        0,
        0,
        -1,
        -1,
        Vector2{0.0f, 0.0f}
    };
    ResetGameWorld(gameWorld);
    DisableCursor();

    return gameWorld;
}

void UnloadGameWorld(GameWorld& gameWorld) {
    EnableCursor();
    if (gameWorld.joltWorld) {
        gameWorld.joltWorld->Shutdown();
    }
    UnloadInteractionSystem(gameWorld.interactions);
    UnloadLevel(gameWorld.level);
    UnloadStaticWorld(gameWorld.world);
}

void UpdateGameplay(GameWorld& gameWorld, float frameDelta) {
    float physicsStep = gameWorld.personConfig.fixedTimeStep;
    gameWorld.physicsAccumulator += frameDelta;

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
        return;
    }

    bool dialogueOpen = gameWorld.interactions.dialogueOpen;
    if (dialogueOpen) {
        gameWorld.physicsAccumulator = 0.0f;
        UpdateDialogueInput(gameWorld.interactions);
        sysState = gameWorld.interactions.dialogueOpen ? SysState::DIALOGUE : SysState::PLAYING;
        ApplyPersonCamera(gameWorld.camera, gameWorld.person, gameWorld.personConfig, frameDelta);
        return;
    }

    sysState = SysState::PLAYING;
    UpdatePersonLook(gameWorld.person, gameWorld.personConfig);
    PersonInput input = ReadPersonInput(true);

    int steps = 0;
    while (gameWorld.physicsAccumulator >= physicsStep && steps < 8) {
        UpdatePersonHorizontalMovement(gameWorld.person, gameWorld.personConfig, input, physicsStep);

        if (gameWorld.joltWorld && gameWorld.joltWorld->IsReady() && gameWorld.joltWorld->HasCharacter()) {
            Vector3 horizontalVelocity = Vector3{gameWorld.person.velocity.x, 0.0f, gameWorld.person.velocity.z};
            gameWorld.joltWorld->UpdateCharacter(gameWorld.person, gameWorld.personConfig, horizontalVelocity, physicsStep);
        } else {
            gameWorld.person.velocity.y -= gameWorld.personConfig.gravity * physicsStep;
            gameWorld.person.position = Vector3Add(
                gameWorld.person.position,
                Vector3Scale(gameWorld.person.velocity, physicsStep));
            ResolvePersonLevelCollisions(gameWorld.level, gameWorld.person, gameWorld.personConfig);
            ApplyPersonGroundSnap(gameWorld.level, gameWorld.person, gameWorld.personConfig);
        }
        ResolveCharacterCollisions(gameWorld.interactions, gameWorld.person.position, gameWorld.personConfig.capsuleRadius);

        gameWorld.physicsAccumulator -= physicsStep;
        ++steps;
    }

    ApplyPersonCamera(gameWorld.camera, gameWorld.person, gameWorld.personConfig, frameDelta);
    UpdateInteractionFocus(
        gameWorld.interactions,
        gameWorld.camera,
        gameWorld.person.position,
        gameWorld.personConfig.interactionDistance,
        gameWorld.personConfig.interactionRayLength);
    if (IsKeyPressed(KEY_E)) {
        BeginFocusedDialogue(gameWorld.interactions);
        if (gameWorld.interactions.dialogueOpen) {
            sysState = SysState::DIALOGUE;
        }
    }
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
    gameWorld.currentLevelRuntimeConfig = LoadLevelRuntimeConfig(gameWorld.currentLevelConfigPath);
    gameWorld.debugUi.levelConfigDirty = false;
    gameWorld.level = LoadLevel(gameWorld.currentLevelPath, gameWorld.currentLevelRuntimeConfig.skyboxPath);
    gameWorld.personConfig = LoadPersonConfig(
        Utils::ResolveProjectPath(PERSON_CONFIG_PATH),
        DefaultPersonConfig());
    gameWorld.person = CreatePersonState(gameWorld.personConfig);
    UnloadInteractionSystem(gameWorld.interactions);
    gameWorld.interactions = LoadInteractionSystem(gameWorld.currentLevelRuntimeConfig.characters);
    ResetGameWorld(gameWorld);
}

void LoadConfiguredLevel(GameWorld& gameWorld, int levelIndex) {
    const LevelConfigEntry* entry = GetLevelConfigEntry(gameWorld.levelsConfig, levelIndex);
    if (entry == nullptr) return;

    UnloadLevel(gameWorld.level);
    gameWorld.currentLevelConfigIndex = levelIndex;
    gameWorld.currentLevelPath = entry->path;
    gameWorld.currentLevelConfigPath = entry->configPath;
    gameWorld.currentLevelRuntimeConfig = LoadLevelRuntimeConfig(gameWorld.currentLevelConfigPath);
    gameWorld.debugUi.levelConfigDirty = false;
    gameWorld.level = LoadLevel(gameWorld.currentLevelPath, gameWorld.currentLevelRuntimeConfig.skyboxPath);
    gameWorld.personConfig = LoadPersonConfig(
        Utils::ResolveProjectPath(PERSON_CONFIG_PATH),
        DefaultPersonConfig());
    gameWorld.person = CreatePersonState(gameWorld.personConfig);
    UnloadInteractionSystem(gameWorld.interactions);
    gameWorld.interactions = LoadInteractionSystem(gameWorld.currentLevelRuntimeConfig.characters);
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
    if (fromIndex >= static_cast<int>(gameWorld.levelsConfig.levels.size())) return;
    if (toIndex >= static_cast<int>(gameWorld.levelsConfig.levels.size())) return;

    std::swap(
        gameWorld.levelsConfig.levels[static_cast<std::size_t>(fromIndex)],
        gameWorld.levelsConfig.levels[static_cast<std::size_t>(toIndex)]);
    gameWorld.debugUi.selectedLevelConfigIndex = toIndex;
    gameWorld.debugUi.levelLoadScroll = Clamp(gameWorld.debugUi.levelLoadScroll, 0, std::max(0, static_cast<int>(gameWorld.levelsConfig.levels.size()) - 1));
    if (gameWorld.currentLevelConfigIndex == fromIndex) {
        gameWorld.currentLevelConfigIndex = toIndex;
    } else if (gameWorld.currentLevelConfigIndex == toIndex) {
        gameWorld.currentLevelConfigIndex = fromIndex;
    }
    SaveLevelsConfigToDisk(gameWorld.levelsConfig);
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

void SetVectorInput(std::string inputs[3], Vector3 value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.x);
    inputs[0] = buffer;
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.y);
    inputs[1] = buffer;
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.z);
    inputs[2] = buffer;
}

bool ParseVectorInput(const std::string inputs[3], Vector3& value) {
    char* end = nullptr;
    value.x = std::strtof(inputs[0].c_str(), &end);
    if (end == inputs[0].c_str()) return false;
    value.y = std::strtof(inputs[1].c_str(), &end);
    if (end == inputs[1].c_str()) return false;
    value.z = std::strtof(inputs[2].c_str(), &end);
    return end != inputs[2].c_str();
}

Quaternion QuaternionFromEulerDegrees(Vector3 degrees) {
    return QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
}

Vector3 EulerDegreesFromQuaternion(Quaternion rotation) {
    Vector3 radians = QuaternionToEuler(rotation);
    return Vector3{radians.x * RAD2DEG, radians.y * RAD2DEG, radians.z * RAD2DEG};
}

void SetLevelNodeInputs(DebugUiState& ui, const LevelDebugNode& node) {
    SetVectorInput(ui.levelPositionInput, node.position);
    SetVectorInput(ui.levelRotationInput, EulerDegreesFromQuaternion(node.rotation));
    SetVectorInput(ui.levelScaleInput, node.scale);
}

int CharacterSelectionId(int index) {
    return -1000 - index;
}

int CharacterIndexFromSelection(int selection) {
    return -1000 - selection;
}

int CharacterPartSelectionId(int characterIndex, int kind, int partIndex) {
    return -200000 - characterIndex * 10000 - kind * 1000 - partIndex;
}

bool DecodeCharacterPartSelection(int selection, int& characterIndex, int& kind, int& partIndex) {
    if (selection > -200000) return false;
    int value = -200000 - selection;
    characterIndex = value / 10000;
    value %= 10000;
    kind = value / 1000;
    partIndex = value % 1000;
    return true;
}

void ApplySelectedLevelNodeInputs(GameWorld& gameWorld) {
    if (gameWorld.debugUi.selectedLevelNode < 0 || gameWorld.debugUi.selectedLevelNode >= static_cast<int>(gameWorld.level.debugNodes.size())) return;
    Vector3 position = Vector3Zero();
    Vector3 rotationDegrees = Vector3Zero();
    Vector3 scale = Vector3{1.0f, 1.0f, 1.0f};
    if (!ParseVectorInput(gameWorld.debugUi.levelPositionInput, position)) return;
    if (!ParseVectorInput(gameWorld.debugUi.levelRotationInput, rotationDegrees)) return;
    if (!ParseVectorInput(gameWorld.debugUi.levelScaleInput, scale)) return;
    ApplyLevelDebugNodeTransform(gameWorld.level, gameWorld.debugUi.selectedLevelNode, position, QuaternionFromEulerDegrees(rotationDegrees), scale);
    ReloadJoltLevelPhysics(gameWorld);
}

bool GetSelectedTransform(GameWorld& gameWorld, Vector3& position, Quaternion& rotation, Vector3& scale, bool& saveToLevelConfig) {
    saveToLevelConfig = false;
    scale = Vector3{1.0f, 1.0f, 1.0f};
    if (gameWorld.debugUi.selectedLevelNode == -2) {
        position = gameWorld.level.rootPosition;
        rotation = gameWorld.level.rootRotation;
        return true;
    }
    if (gameWorld.debugUi.selectedLevelNode <= -1000 && gameWorld.debugUi.selectedLevelNode > -200000) {
        int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (characterIndex < 0 || characterIndex >= static_cast<int>(gameWorld.interactions.characters.size())) return false;
        position = gameWorld.interactions.characters[characterIndex].rootPosition;
        rotation = gameWorld.interactions.characters[characterIndex].rootRotation;
        saveToLevelConfig = true;
        return true;
    }
    if (gameWorld.debugUi.selectedLevelNode >= 0 && gameWorld.debugUi.selectedLevelNode < static_cast<int>(gameWorld.level.debugNodes.size())) {
        const LevelDebugNode& node = gameWorld.level.debugNodes[gameWorld.debugUi.selectedLevelNode];
        position = node.position;
        rotation = node.rotation;
        scale = node.scale;
        return node.kind != LevelDebugNodeKind::Visual;
    }
    return false;
}

void SetSelectedTransform(GameWorld& gameWorld, Vector3 position, Quaternion rotation, Vector3 scale) {
    if (gameWorld.debugUi.selectedLevelNode == -2) {
        ApplyLevelRootTransform(gameWorld.level, position, rotation);
        ReloadJoltLevelPhysics(gameWorld);
    } else if (gameWorld.debugUi.selectedLevelNode <= -1000 && gameWorld.debugUi.selectedLevelNode > -200000) {
        int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (characterIndex >= 0 && characterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
            ApplyCharacterRootTransform(gameWorld.interactions.characters[characterIndex], position, rotation);
            gameWorld.debugUi.levelConfigDirty = true;
        }
    } else if (gameWorld.debugUi.selectedLevelNode >= 0 && gameWorld.debugUi.selectedLevelNode < static_cast<int>(gameWorld.level.debugNodes.size())) {
        ApplyLevelDebugNodeTransform(gameWorld.level, gameWorld.debugUi.selectedLevelNode, position, rotation, scale);
        ReloadJoltLevelPhysics(gameWorld);
    }
    SetVectorInput(gameWorld.debugUi.levelPositionInput, position);
    SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(rotation));
    SetVectorInput(gameWorld.debugUi.levelScaleInput, scale);
}

void SaveCurrentLevelRuntimeConfig(GameWorld& gameWorld) {
    for (std::size_t i = 0; i < gameWorld.currentLevelRuntimeConfig.characters.size() && i < gameWorld.interactions.characters.size(); ++i) {
        gameWorld.currentLevelRuntimeConfig.characters[i].position = gameWorld.interactions.characters[i].rootPosition;
        gameWorld.currentLevelRuntimeConfig.characters[i].rotationDegrees = EulerDegreesFromQuaternion(gameWorld.interactions.characters[i].rootRotation);
    }
    if (SaveLevelRuntimeConfig(gameWorld.currentLevelConfigPath, gameWorld.currentLevelRuntimeConfig)) {
        gameWorld.debugUi.levelConfigDirty = false;
    }
}

void ReloadCurrentLevelForConfig(GameWorld& gameWorld) {
    UnloadLevel(gameWorld.level);
    gameWorld.level = LoadLevel(gameWorld.currentLevelPath, gameWorld.currentLevelRuntimeConfig.skyboxPath);
    ResetGameWorld(gameWorld);
}

bool DebugTextInput(Rectangle rect, const char* label, std::string& text, int fieldId, DebugUiState& ui) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.activeTextField = fieldId;
    }

    bool active = ui.activeTextField == fieldId;
    bool changed = false;
    DrawText(label, static_cast<int>(rect.x), static_cast<int>(rect.y + 5), 16, RAYWHITE);
    Rectangle inputRect = Rectangle{rect.x + 24.0f, rect.y, rect.width - 24.0f, rect.height};
    DrawRectangleRec(inputRect, active ? Color{46, 56, 72, 255} : Color{36, 36, 42, 255});
    DrawRectangleLinesEx(inputRect, 1.0f, active ? Color{120, 150, 220, 255} : Color{85, 85, 95, 255});
    DrawText(text.c_str(), static_cast<int>(inputRect.x + 6.0f), static_cast<int>(inputRect.y + 5.0f), 16, RAYWHITE);

    if (active) {
        int key = GetCharPressed();
        while (key > 0) {
            char c = static_cast<char>(key);
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
                if (text.size() < 20) {
                    text.push_back(c);
                    changed = true;
                }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) {
            text.pop_back();
            changed = true;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            ui.activeTextField = 0;
        }
    }
    return changed;
}

void TeleportDebugCamera(GameWorld& gameWorld, Vector3 position) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(gameWorld.camera.target, gameWorld.camera.position));
    if (Vector3LengthSqr(forward) <= 0.0001f) forward = Vector3{0.0f, 0.0f, -1.0f};
    gameWorld.camera.position = position;
    gameWorld.camera.target = Vector3Add(position, forward);
    SyncDebugCameraRotation(gameWorld.debugCamera, gameWorld.camera);
}

void TeleportPerson(GameWorld& gameWorld, Vector3 position) {
    gameWorld.person.position = position;
    gameWorld.person.velocity = Vector3Zero();
    gameWorld.person.grounded = false;
    ApplyPersonCamera(gameWorld.camera, gameWorld.person, gameWorld.personConfig, 1.0f);
}

void TeleportVehicle(GameWorld& gameWorld, Vector3 position) {
    gameWorld.vehicle.position = position;
    gameWorld.vehicle.linearVelocity = Vector3Zero();
    gameWorld.vehicle.angularVelocity = Vector3Zero();
    if (gameWorld.vehicleBody != nullptr) {
        SyncBodyFromVehicleState(*gameWorld.vehicleBody, gameWorld.vehicle);
    }
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

void DrawTransformGizmo(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled || !gameWorld.debugUi.levelSidebarOpen || gameWorld.debugUi.transformGizmoMode == 0) return;

    Vector3 position = Vector3Zero();
    Quaternion rotation = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 scale = Vector3{1.0f, 1.0f, 1.0f};
    bool saveToConfig = false;
    if (!GetSelectedTransform(gameWorld, position, rotation, scale, saveToConfig)) return;

    const Vector3 axes[3] = {
        Vector3{1.0f, 0.0f, 0.0f},
        Vector3{0.0f, 1.0f, 0.0f},
        Vector3{0.0f, 0.0f, 1.0f}
    };
    const Color colors[3] = {RED, GREEN, BLUE};
    Vector2 mouse = GetMousePosition();

    float bestDistance = 99999.0f;
    int hoveredAxis = -1;
    for (int i = 0; i < 3; ++i) {
        Vector3 axis = gameWorld.debugUi.transformGizmoMode == 2 ? Vector3RotateByQuaternion(axes[i], rotation) : axes[i];
        Vector3 end = Vector3Add(position, Vector3Scale(axis, 1.6f));
        DrawLine3D(position, end, colors[i]);
        DrawSphere(end, 0.08f, colors[i]);
        Vector2 screenEnd = GetWorldToScreen(end, gameWorld.camera);
        float distance = Vector2Distance(mouse, screenEnd);
        if (distance < bestDistance && distance < 18.0f) {
            bestDistance = distance;
            hoveredAxis = i;
        }
    }

    if (!gameWorld.debugUi.draggingTransformGizmo && hoveredAxis >= 0 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        gameWorld.debugUi.draggingTransformGizmo = true;
        gameWorld.debugUi.transformGizmoAxis = hoveredAxis;
        gameWorld.debugUi.transformGizmoLastMouse = mouse;
    }
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        gameWorld.debugUi.draggingTransformGizmo = false;
        gameWorld.debugUi.transformGizmoAxis = -1;
    }
    if (!gameWorld.debugUi.draggingTransformGizmo || gameWorld.debugUi.transformGizmoAxis < 0) return;

    Vector2 delta = Vector2Subtract(mouse, gameWorld.debugUi.transformGizmoLastMouse);
    gameWorld.debugUi.transformGizmoLastMouse = mouse;
    int axisIndex = gameWorld.debugUi.transformGizmoAxis;
    Vector3 axis = gameWorld.debugUi.transformGizmoMode == 2 ? Vector3RotateByQuaternion(axes[axisIndex], rotation) : axes[axisIndex];
    Vector2 screenPos = GetWorldToScreen(position, gameWorld.camera);
    Vector2 screenAxis = Vector2Subtract(GetWorldToScreen(Vector3Add(position, axis), gameWorld.camera), screenPos);
    float screenAxisLen = Vector2Length(screenAxis);
    if (screenAxisLen <= 0.001f) return;
    screenAxis = Vector2Scale(screenAxis, 1.0f / screenAxisLen);
    float mouseAlongAxis = Vector2DotProduct(delta, screenAxis);

    if (gameWorld.debugUi.transformGizmoMode == 1) {
        position = Vector3Add(position, Vector3Scale(axis, mouseAlongAxis * 0.025f));
    } else if (gameWorld.debugUi.transformGizmoMode == 2) {
        Quaternion deltaRotation = QuaternionFromAxisAngle(axis, mouseAlongAxis * 0.01f);
        rotation = QuaternionNormalize(QuaternionMultiply(deltaRotation, rotation));
    }
    SetSelectedTransform(gameWorld, position, rotation, scale);
}

bool LevelNodeMatchesTab(const LevelDebugNode& node, int tab) {
    if (tab == 1) return node.kind == LevelDebugNodeKind::Visual;
    if (tab == 2) return node.kind == LevelDebugNodeKind::Collider;
    if (tab == 3) return node.kind == LevelDebugNodeKind::Icon;
    if (tab == 4) return node.kind == LevelDebugNodeKind::Spawn || node.kind == LevelDebugNodeKind::Trigger;
    if (tab == 5) return node.kind == LevelDebugNodeKind::Invisible || node.kind == LevelDebugNodeKind::Other;
    return false;
}

void DrawLevelSidebar(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled || !gameWorld.debugUi.levelSidebarOpen) return;

    float width = 360.0f;
    float x = static_cast<float>(GetScreenWidth()) - width;
    float h = static_cast<float>(GetScreenHeight()) - 34.0f;
    Rectangle panel = Rectangle{x, 34.0f, width, h};
    DrawRectangleRec(panel, Color{24, 24, 30, 235});
    DrawRectangleLinesEx(panel, 1.0f, Color{80, 80, 88, 255});
    gameWorld.debugUi.levelSidebarTab = 0;
    DrawText("Inspector", static_cast<int>(x + 14.0f), 44, 20, RAYWHITE);

    const char* tabs[] = {"ROOT"};
    for (int i = 0; i < 1; ++i) {
        Rectangle tab = Rectangle{x + 12.0f + i * 56.0f, 74.0f, 52.0f, 26.0f};
        bool active = gameWorld.debugUi.levelSidebarTab == i;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), tab);
        DrawRectangleRec(tab, active ? Color{78, 92, 120, 255} : hovered ? Color{64, 64, 72, 255} : Color{42, 42, 48, 255});
        DrawRectangleLinesEx(tab, 1.0f, Color{85, 85, 95, 255});
        DrawText(tabs[i], static_cast<int>(tab.x + 5.0f), static_cast<int>(tab.y + 6.0f), 13, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.levelSidebarTab = i;
            gameWorld.debugUi.levelSidebarScroll = 0;
        }
    }

    if (gameWorld.debugUi.levelSidebarTab == 0) {
        float rootY = 110.0f;
        Rectangle rootRow = Rectangle{x + 12.0f, rootY, width - 24.0f, 24.0f};
        bool selected = gameWorld.debugUi.selectedLevelNode == -2;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rootRow);
        DrawRectangleRec(rootRow, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
        std::string levelName = DisplayNameFromPath(gameWorld.level.name);
        DrawText(TextFormat("LEVEL %s", levelName.c_str()), static_cast<int>(rootRow.x + 6.0f), static_cast<int>(rootRow.y + 5.0f), 14, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.selectedLevelNode = -2;
            SetVectorInput(gameWorld.debugUi.levelPositionInput, gameWorld.level.rootPosition);
            SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(gameWorld.level.rootRotation));
        }
        rootY += 28.0f;
        for (std::size_t c = 0; c < gameWorld.interactions.characters.size(); ++c) {
            const InteractableCharacter& character = gameWorld.interactions.characters[c];
            Rectangle row = Rectangle{x + 12.0f, rootY, width - 24.0f, 24.0f};
            int selectionId = CharacterSelectionId(static_cast<int>(c));
            selected = gameWorld.debugUi.selectedLevelNode == selectionId;
            hovered = CheckCollisionPointRec(GetMousePosition(), row);
            DrawRectangleRec(row, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
            std::string characterName = DisplayNameFromPath(character.modelPath);
            DrawText(TextFormat("CHAR %s", characterName.c_str()), static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 5.0f), 14, RAYWHITE);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                gameWorld.debugUi.selectedLevelNode = selectionId;
                SetVectorInput(gameWorld.debugUi.levelPositionInput, character.rootPosition);
                SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(character.rootRotation));
            }
            rootY += 28.0f;
        }
    }

    int matchingCount = 0;
    for (std::size_t i = 0; i < gameWorld.level.debugNodes.size(); ++i) {
        if (LevelNodeMatchesTab(gameWorld.level.debugNodes[i], gameWorld.debugUi.levelSidebarTab)) ++matchingCount;
    }
    for (std::size_t c = 0; c < gameWorld.interactions.characters.size(); ++c) {
        const InteractableCharacter& character = gameWorld.interactions.characters[c];
        if (gameWorld.debugUi.levelSidebarTab == 1) matchingCount += static_cast<int>(character.visualParts.size());
        if (gameWorld.debugUi.levelSidebarTab == 2) matchingCount += static_cast<int>(character.colliders.size());
        if (gameWorld.debugUi.levelSidebarTab == 3) matchingCount += static_cast<int>(character.iconParts.size());
    }

    float listY = 110.0f;
    float rowH = 22.0f;
    int visibleRows = 14;
    if (CheckCollisionPointRec(GetMousePosition(), Rectangle{x, listY, width, visibleRows * rowH})) {
        gameWorld.debugUi.levelSidebarScroll -= static_cast<int>(GetMouseWheelMove());
        gameWorld.debugUi.levelSidebarScroll = Clamp(gameWorld.debugUi.levelSidebarScroll, 0, std::max(0, matchingCount - visibleRows));
    }

    int skipped = 0;
    int drawn = 0;
    for (std::size_t i = 0; i < gameWorld.level.debugNodes.size() && drawn < visibleRows; ++i) {
        const LevelDebugNode& node = gameWorld.level.debugNodes[i];
        if (!LevelNodeMatchesTab(node, gameWorld.debugUi.levelSidebarTab)) continue;
        if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
        Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
        bool selected = gameWorld.debugUi.selectedLevelNode == static_cast<int>(i);
        bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
        DrawRectangleRec(row, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
        std::string nodeName = DisplayNameFromPath(node.name);
        DrawText(TextFormat("LEVEL %s", nodeName.c_str()), static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.selectedLevelNode = static_cast<int>(i);
            SetLevelNodeInputs(gameWorld.debugUi, node);
        }
        ++drawn;
    }
    for (std::size_t c = 0; c < gameWorld.interactions.characters.size() && drawn < visibleRows; ++c) {
        const InteractableCharacter& character = gameWorld.interactions.characters[c];
        if (gameWorld.debugUi.levelSidebarTab == 1) {
            for (std::size_t p = 0; p < character.visualParts.size() && drawn < visibleRows; ++p) {
                if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
                int selectionId = CharacterPartSelectionId(static_cast<int>(c), 0, static_cast<int>(p));
                Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
                bool selected = gameWorld.debugUi.selectedLevelNode == selectionId;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
                DrawRectangleRec(row, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
                std::string characterName = DisplayNameFromPath(character.modelPath);
                DrawText(TextFormat("CHAR %s/%s", characterName.c_str(), character.visualParts[p].name.c_str()), static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) gameWorld.debugUi.selectedLevelNode = selectionId;
                ++drawn;
            }
        } else if (gameWorld.debugUi.levelSidebarTab == 2) {
            for (std::size_t p = 0; p < character.colliders.size() && drawn < visibleRows; ++p) {
                if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
                int selectionId = CharacterPartSelectionId(static_cast<int>(c), 1, static_cast<int>(p));
                Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
                bool selected = gameWorld.debugUi.selectedLevelNode == selectionId;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
                DrawRectangleRec(row, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
                std::string characterName = DisplayNameFromPath(character.modelPath);
                DrawText(TextFormat("CHAR %s/%s", characterName.c_str(), character.colliders[p].name.c_str()), static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) gameWorld.debugUi.selectedLevelNode = selectionId;
                ++drawn;
            }
        } else if (gameWorld.debugUi.levelSidebarTab == 3) {
            for (std::size_t p = 0; p < character.iconParts.size() && drawn < visibleRows; ++p) {
                if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
                int selectionId = CharacterPartSelectionId(static_cast<int>(c), 2, static_cast<int>(p));
                Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
                bool selected = gameWorld.debugUi.selectedLevelNode == selectionId;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
                DrawRectangleRec(row, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
                std::string characterName = DisplayNameFromPath(character.modelPath);
                DrawText(TextFormat("CHAR %s/%s", characterName.c_str(), character.iconParts[p].name.c_str()), static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) gameWorld.debugUi.selectedLevelNode = selectionId;
                ++drawn;
            }
        }
    }
    DrawText(TextFormat("%d items", matchingCount), static_cast<int>(x + 14.0f), static_cast<int>(listY + visibleRows * rowH + 4.0f), 14, GRAY);

    float editY = listY + visibleRows * rowH + 24.0f;
    if (gameWorld.debugUi.selectedLevelNode == -2 || gameWorld.debugUi.selectedLevelNode <= -1000) {
        bool editingCharacter = gameWorld.debugUi.selectedLevelNode <= -1000;
        DrawText(editingCharacter ? "CHARACTER ROOT" : "LEVEL ROOT", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
        DrawText("Move/rotate GLB inteiro. Scale fica para depois.", static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE); editY += 26.0f;

        bool changed = false;
        DrawText("Position", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelPositionInput[0], 401, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelPositionInput[1], 402, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelPositionInput[2], 403, gameWorld.debugUi) || changed;
        editY += 34.0f;

        DrawText("Rotation", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelRotationInput[0], 404, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelRotationInput[1], 405, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelRotationInput[2], 406, gameWorld.debugUi) || changed;
        editY += 34.0f;
        if (DebugButton(Rectangle{x + 14.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 1 ? "Move ON" : "Move")) gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 1 ? 0 : 1;
        if (DebugButton(Rectangle{x + 120.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 2 ? "Rotate ON" : "Rotate")) gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 2 ? 0 : 2;
        editY += 34.0f;

        if (changed) {
            Vector3 position = Vector3Zero();
            Vector3 rotationDegrees = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.levelPositionInput, position) && ParseVectorInput(gameWorld.debugUi.levelRotationInput, rotationDegrees)) {
                if (editingCharacter) {
                    int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
                    if (characterIndex >= 0 && characterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
                        ApplyCharacterRootTransform(gameWorld.interactions.characters[characterIndex], position, QuaternionFromEulerDegrees(rotationDegrees));
                        gameWorld.debugUi.levelConfigDirty = true;
                    }
                } else {
                    ApplyLevelRootTransform(gameWorld.level, position, QuaternionFromEulerDegrees(rotationDegrees));
                    ReloadJoltLevelPhysics(gameWorld);
                }
            }
        }
        if (DebugButton(Rectangle{x + 14.0f, editY, 190.0f, 26.0f}, "Teleport to Camera")) {
            if (editingCharacter) {
                int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
                if (characterIndex >= 0 && characterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
                    ApplyCharacterRootTransform(gameWorld.interactions.characters[characterIndex], gameWorld.camera.position, gameWorld.interactions.characters[characterIndex].rootRotation);
                    gameWorld.debugUi.levelConfigDirty = true;
                    SetVectorInput(gameWorld.debugUi.levelPositionInput, gameWorld.interactions.characters[characterIndex].rootPosition);
                }
            } else {
                ApplyLevelRootTransform(gameWorld.level, gameWorld.camera.position, gameWorld.level.rootRotation);
                SetVectorInput(gameWorld.debugUi.levelPositionInput, gameWorld.level.rootPosition);
                ReloadJoltLevelPhysics(gameWorld);
            }
        }
        if (editingCharacter) {
            editY += 34.0f;
            if (DebugButton(Rectangle{x + 14.0f, editY, 190.0f, 26.0f}, gameWorld.debugUi.levelConfigDirty ? "Save car_meet.json *" : "Save car_meet.json")) {
                SaveCurrentLevelRuntimeConfig(gameWorld);
            }
            editY += 40.0f;
            const char* detailTabs[] = {"VISUAL", "COL", "ICON", "SP/TR", "OTHER"};
            for (int i = 0; i < 5; ++i) {
                Rectangle tab = Rectangle{x + 12.0f + i * 66.0f, editY, 62.0f, 24.0f};
                bool active = gameWorld.debugUi.inspectorDetailTab == i;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), tab);
                DrawRectangleRec(tab, active ? Color{78, 92, 120, 255} : hovered ? Color{64, 64, 72, 255} : Color{42, 42, 48, 255});
                DrawText(detailTabs[i], static_cast<int>(tab.x + 4.0f), static_cast<int>(tab.y + 6.0f), 12, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) gameWorld.debugUi.inspectorDetailTab = i;
            }
            editY += 30.0f;
            int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
            if (characterIndex >= 0 && characterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
                const InteractableCharacter& character = gameWorld.interactions.characters[characterIndex];
                if (gameWorld.debugUi.inspectorDetailTab == 0) {
                    for (std::size_t i = 0; i < character.visualParts.size() && editY < h - 24.0f; ++i, editY += 20.0f) DrawText(character.visualParts[i].name.c_str(), static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, LIGHTGRAY);
                } else if (gameWorld.debugUi.inspectorDetailTab == 1) {
                    for (std::size_t i = 0; i < character.colliders.size() && editY < h - 24.0f; ++i, editY += 20.0f) DrawText(character.colliders[i].name.c_str(), static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, LIGHTGRAY);
                } else if (gameWorld.debugUi.inspectorDetailTab == 2) {
                    for (std::size_t i = 0; i < character.iconParts.size() && editY < h - 24.0f; ++i, editY += 20.0f) DrawText(character.iconParts[i].name.c_str(), static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, LIGHTGRAY);
                } else {
                    DrawText("Sem dados nesta aba para CHAR.", static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, GRAY);
                }
            }
        }
    }

    int selectedCharacterIndex = 0;
    int selectedCharacterPartKind = 0;
    int selectedCharacterPartIndex = 0;
    if (DecodeCharacterPartSelection(gameWorld.debugUi.selectedLevelNode, selectedCharacterIndex, selectedCharacterPartKind, selectedCharacterPartIndex)) {
        if (selectedCharacterIndex >= 0 && selectedCharacterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
            const InteractableCharacter& character = gameWorld.interactions.characters[selectedCharacterIndex];
            const char* kindName = selectedCharacterPartKind == 0 ? "VISUAL" : selectedCharacterPartKind == 1 ? "COL" : "ICON";
            const char* partName = "";
            if (selectedCharacterPartKind == 0 && selectedCharacterPartIndex < static_cast<int>(character.visualParts.size())) partName = character.visualParts[selectedCharacterPartIndex].name.c_str();
            if (selectedCharacterPartKind == 1 && selectedCharacterPartIndex < static_cast<int>(character.colliders.size())) partName = character.colliders[selectedCharacterPartIndex].name.c_str();
            if (selectedCharacterPartKind == 2 && selectedCharacterPartIndex < static_cast<int>(character.iconParts.size())) partName = character.iconParts[selectedCharacterPartIndex].name.c_str();
            std::string characterName = DisplayNameFromPath(character.modelPath);
            DrawText(TextFormat("CHAR %s", characterName.c_str()), static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
            DrawText(TextFormat("%s %s", kindName, partName), static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, LIGHTGRAY); editY += 22.0f;
            DrawText("Part read-only. Use ROOT/CHAR to move whole GLB.", static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE);
        }
    }

    if (gameWorld.debugUi.selectedLevelNode >= 0 && gameWorld.debugUi.selectedLevelNode < static_cast<int>(gameWorld.level.debugNodes.size())) {
        const LevelDebugNode& node = gameWorld.level.debugNodes[gameWorld.debugUi.selectedLevelNode];
        std::string nodeName = DisplayNameFromPath(node.name);
        DrawText(nodeName.c_str(), static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
        DrawText(LevelDebugNodeKindName(node.kind), static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, GRAY); editY += 24.0f;
        if (node.kind == LevelDebugNodeKind::Visual) {
            DrawText("VISUAL read-only: level render voltou para DrawModel().", static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE);
            editY += 22.0f;
        }

        DrawText("Position", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        bool changed = false;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelPositionInput[0], 301, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelPositionInput[1], 302, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelPositionInput[2], 303, gameWorld.debugUi) || changed;
        editY += 34.0f;

        DrawText("Rotation", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelRotationInput[0], 304, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelRotationInput[1], 305, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelRotationInput[2], 306, gameWorld.debugUi) || changed;
        editY += 34.0f;
        if (DebugButton(Rectangle{x + 14.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 1 ? "Move ON" : "Move")) gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 1 ? 0 : 1;
        if (DebugButton(Rectangle{x + 120.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 2 ? "Rotate ON" : "Rotate")) gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 2 ? 0 : 2;
        editY += 34.0f;

        DrawText("Scale", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelScaleInput[0], 307, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelScaleInput[1], 308, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelScaleInput[2], 309, gameWorld.debugUi) || changed;
        editY += 38.0f;

        if (changed && node.kind != LevelDebugNodeKind::Visual) ApplySelectedLevelNodeInputs(gameWorld);
        if (node.kind != LevelDebugNodeKind::Visual && DebugButton(Rectangle{x + 14.0f, editY, 190.0f, 26.0f}, "Teleport to Camera")) {
            TeleportLevelDebugNodeToCamera(gameWorld.level, gameWorld.debugUi.selectedLevelNode, gameWorld.camera);
            SetLevelNodeInputs(gameWorld.debugUi, gameWorld.level.debugNodes[gameWorld.debugUi.selectedLevelNode]);
            ReloadJoltLevelPhysics(gameWorld);
        }
    }
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
        BeginDebugPanel(gameWorld.debugUi, 0, pos, gameWorld.debugUi.pinVehicleStatus, "Person Status", 280.0f, 132.0f);
        float speed = Vector3Length(Vector3{gameWorld.person.velocity.x, 0.0f, gameWorld.person.velocity.z});
        DrawText(TextFormat("Speed: %.2f m/s", speed), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 44), 18, RAYWHITE);
        DrawText(gameWorld.person.grounded ? "Grounded" : "Airborne", static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 68), 18, gameWorld.person.grounded ? GREEN : RED);
        DrawText(TextFormat("Pos: %.1f %.1f %.1f", gameWorld.person.position.x, gameWorld.person.position.y, gameWorld.person.position.z), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 92), 18, RAYWHITE);
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

    if (gameWorld.debugUi.debugTeleportOpen) {
        Vector2& pos = gameWorld.debugUi.debugTeleportPos;
        bool pinned = false;
        BeginDebugPanel(gameWorld.debugUi, 3, pos, pinned, "Debug Camera Teleport", 320.0f, 150.0f);
        DrawText(TextFormat("Camera: %.2f %.2f %.2f", gameWorld.camera.position.x, gameWorld.camera.position.y, gameWorld.camera.position.z), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 38), 16, LIGHTGRAY);
        DebugTextInput(Rectangle{pos.x + 14.0f, pos.y + 64.0f, 88.0f, 26.0f}, "X", gameWorld.debugUi.debugTeleportInput[0], 101, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 112.0f, pos.y + 64.0f, 88.0f, 26.0f}, "Y", gameWorld.debugUi.debugTeleportInput[1], 102, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 210.0f, pos.y + 64.0f, 88.0f, 26.0f}, "Z", gameWorld.debugUi.debugTeleportInput[2], 103, gameWorld.debugUi);
        if (DebugButton(Rectangle{pos.x + 14.0f, pos.y + 108.0f, 140.0f, 26.0f}, "Teleport")) {
            Vector3 target = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.debugTeleportInput, target)) TeleportDebugCamera(gameWorld, target);
        }
        if (DebugButton(Rectangle{pos.x + 164.0f, pos.y + 108.0f, 70.0f, 26.0f}, "Use cam")) {
            SetVectorInput(gameWorld.debugUi.debugTeleportInput, gameWorld.camera.position);
        }
        if (DebugButton(Rectangle{pos.x + 244.0f, pos.y + 108.0f, 60.0f, 26.0f}, "Close")) {
            gameWorld.debugUi.debugTeleportOpen = false;
            gameWorld.debugUi.activeTextField = 0;
        }
    }

    if (gameWorld.debugUi.gameTeleportOpen) {
        Vector2& pos = gameWorld.debugUi.gameTeleportPos;
        bool pinned = false;
        BeginDebugPanel(gameWorld.debugUi, 4, pos, pinned, "Game Teleport", 320.0f, 162.0f);
        DrawText(TextFormat("Person: %.2f %.2f %.2f", gameWorld.person.position.x, gameWorld.person.position.y, gameWorld.person.position.z), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 38), 16, LIGHTGRAY);
        DrawText(TextFormat("Vehicle: %.2f %.2f %.2f", gameWorld.vehicle.position.x, gameWorld.vehicle.position.y, gameWorld.vehicle.position.z), static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 58), 16, LIGHTGRAY);
        DebugTextInput(Rectangle{pos.x + 14.0f, pos.y + 84.0f, 88.0f, 26.0f}, "X", gameWorld.debugUi.gameTeleportInput[0], 201, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 112.0f, pos.y + 84.0f, 88.0f, 26.0f}, "Y", gameWorld.debugUi.gameTeleportInput[1], 202, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 210.0f, pos.y + 84.0f, 88.0f, 26.0f}, "Z", gameWorld.debugUi.gameTeleportInput[2], 203, gameWorld.debugUi);
        if (DebugButton(Rectangle{pos.x + 14.0f, pos.y + 122.0f, 130.0f, 26.0f}, "Teleport Person")) {
            Vector3 target = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.gameTeleportInput, target)) TeleportPerson(gameWorld, target);
        }
        if (DebugButton(Rectangle{pos.x + 154.0f, pos.y + 122.0f, 128.0f, 26.0f}, "Teleport Car")) {
            Vector3 target = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.gameTeleportInput, target)) TeleportVehicle(gameWorld, target);
        }
        if (DebugButton(Rectangle{pos.x + 286.0f, pos.y + 122.0f, 26.0f, 26.0f}, "X")) {
            gameWorld.debugUi.gameTeleportOpen = false;
            gameWorld.debugUi.activeTextField = 0;
        }
    }
}

void DrawGameplay(GameWorld& gameWorld) {
    BeginMode3D(gameWorld.camera);
    DrawLevelSkybox(gameWorld.level, gameWorld.camera);
    DrawLevel(gameWorld.level);
    DrawInteractableCharacters(gameWorld.interactions);
    if (gameWorld.debugUi.showForces) {
        DrawLevelCollidersDebug(gameWorld.level);
        DrawPersonDebugCapsule(gameWorld.person, gameWorld.personConfig);
        Vector3 feetPosition = gameWorld.person.position;
        DrawSphere(feetPosition, 0.05f, gameWorld.person.grounded ? GREEN : RED);
    }
    if (gameWorld.debugUi.levelSidebarOpen) {
        if (gameWorld.debugUi.selectedLevelNode == -2) {
            Vector3 p = gameWorld.level.rootPosition;
            DrawLine3D(p, Vector3Add(p, Vector3{1.0f, 0.0f, 0.0f}), RED);
            DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 1.0f, 0.0f}), GREEN);
            DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 0.0f, 1.0f}), BLUE);
        } else if (gameWorld.debugUi.selectedLevelNode <= -1000 && gameWorld.debugUi.selectedLevelNode > -200000) {
            int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
            if (characterIndex >= 0 && characterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
                DrawCharacterDebugSelection(gameWorld.interactions.characters[characterIndex]);
            }
        } else if (gameWorld.debugUi.selectedLevelNode <= -200000) {
            int characterIndex = 0;
            int kind = 0;
            int partIndex = 0;
            if (DecodeCharacterPartSelection(gameWorld.debugUi.selectedLevelNode, characterIndex, kind, partIndex) &&
                characterIndex >= 0 && characterIndex < static_cast<int>(gameWorld.interactions.characters.size())) {
                const InteractableCharacter& character = gameWorld.interactions.characters[characterIndex];
                if (kind == 1 && partIndex >= 0 && partIndex < static_cast<int>(character.colliders.size())) {
                    const CharacterCapsule& capsule = character.colliders[partIndex];
                    DrawCapsuleWires(capsule.bottom, capsule.top, capsule.radius, 12, 6, ORANGE);
                    Vector3 p = Vector3Scale(Vector3Add(capsule.bottom, capsule.top), 0.5f);
                    DrawLine3D(p, Vector3Add(p, Vector3{1.0f, 0.0f, 0.0f}), RED);
                    DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 1.0f, 0.0f}), GREEN);
                    DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 0.0f, 1.0f}), BLUE);
                } else {
                    DrawCharacterDebugSelection(character);
                }
            }
        } else {
            DrawLevelDebugSelection(gameWorld.level, gameWorld.debugUi.selectedLevelNode, gameWorld.camera);
        }
        DrawTransformGizmo(gameWorld);
    }
    EndMode3D();
    debug_ui::DrawTopBar(gameWorld, debug_ui::TopBarActions{RestartLevel, ResetGameWorld});
    DrawDebugPanels(gameWorld);
    DrawLevelSidebar(gameWorld);
    debug_ui::DrawLevelConfigSidebar(gameWorld, debug_ui::LevelConfigActions{
        SaveCurrentLevelRuntimeConfig,
        ReloadCurrentLevelForConfig,
        LoadConfiguredLevel,
        MoveLevelConfigEntry
    });
    DrawInteractionUi(gameWorld.interactions);
    if (gameWorld.debugUi.enabled) {
        DrawDebugAxisGizmo(gameWorld.camera);
    }
}

}  // namespace

int main() {
    InitWindow(SCRWIDTH, SCRHEIGHT, "Dragon Rage");
    SetExitKey(KEY_NULL);
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
                DisableCursor();
                ApplyPersonCamera(
                    gameWorld.camera,
                    gameWorld.person,
                    gameWorld.personConfig,
                    1.0f / 60.0f);
            }
        }

        UpdateGameplay(gameWorld, GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        DrawGameplay(gameWorld);
        EndDrawing();
    }

    UnloadGameWorld(gameWorld);
    CloseWindow();

    return 0;
}
