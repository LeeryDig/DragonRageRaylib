#include "vehiclePhysics.hpp"

#include <cmath>
#include <cstdlib>
#include <string>

#include "raymath.h"

namespace {

const float kBaseGravity = 9.81f;

std::string ExtractArrayBlock(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t blockStart = json.find('[', keyPos);
    if (blockStart == std::string::npos) {
        return "";
    }

    int depth = 0;
    for (std::size_t i = blockStart; i < json.size(); ++i) {
        if (json[i] == '[') {
            depth++;
        } else if (json[i] == ']') {
            depth--;
            if (depth == 0) {
                return json.substr(blockStart, i - blockStart + 1);
            }
        }
    }

    return "";
}

std::vector<float> ExtractNumbers(const std::string& text) {
    std::vector<float> values;

    const char* cursor = text.c_str();
    char* endCursor = nullptr;
    while (*cursor != '\0') {
        float value = strtof(cursor, &endCursor);
        if (endCursor != cursor) {
            values.push_back(value);
            cursor = endCursor;
        } else {
            ++cursor;
        }
    }

    return values;
}

float ExtractFloat(const std::string& json, const std::string& key, float fallbackValue) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return fallbackValue;
    }

    std::size_t valueStart = json.find(':', keyPos);
    if (valueStart == std::string::npos) {
        return fallbackValue;
    }

    char* endCursor = nullptr;
    float value = strtof(json.c_str() + valueStart + 1, &endCursor);
    return endCursor == json.c_str() + valueStart + 1 ? fallbackValue : value;
}

Vector3 ExtractVector3(const std::string& json, const std::string& key, Vector3 fallbackValue) {
    std::vector<float> values = ExtractNumbers(ExtractArrayBlock(json, key));
    if (values.size() < 3) {
        return fallbackValue;
    }

    return Vector3{values[0], values[1], values[2]};
}

std::array<Vector3, 4> ExtractWheelOffsets(
    const std::string& json,
    const std::string& key,
    const std::array<Vector3, 4>& fallbackOffsets) {
    std::array<Vector3, 4> offsets = fallbackOffsets;
    std::vector<float> values = ExtractNumbers(ExtractArrayBlock(json, key));
    if (values.size() < 12) {
        return offsets;
    }

    for (int i = 0; i < 4; ++i) {
        offsets[i] = Vector3{values[i * 3], values[i * 3 + 1], values[i * 3 + 2]};
    }

    return offsets;
}

std::vector<CurvePoint> ExtractCurve(
    const std::string& json,
    const std::string& key,
    const std::vector<CurvePoint>& fallbackCurve) {
    std::vector<float> values = ExtractNumbers(ExtractArrayBlock(json, key));
    if (values.size() < 2 || values.size() % 2 != 0) {
        return fallbackCurve;
    }

    std::vector<CurvePoint> curve;
    curve.reserve(values.size() / 2);
    for (std::size_t i = 0; i < values.size(); i += 2) {
        curve.push_back(CurvePoint{values[i], values[i + 1]});
    }
    return curve;
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

float MoveTowardsScalar(float value, float target, float maxDelta) {
    if (fabsf(target - value) <= maxDelta) {
        return target;
    }

    return value + (target > value ? maxDelta : -maxDelta);
}

float SignNonZero(float value) {
    if (value > 0.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return -1.0f;
    }
    return 0.0f;
}

Vector3 LocalDirectionToWorld(const VehicleState& state, const Vector3& localDirection) {
    return Vector3RotateByQuaternion(localDirection, state.rotation);
}

Vector3 GetCenterOfMassWorld(const VehicleState& state, const VehicleConfig& config) {
    return Vector3Add(state.position, Vector3RotateByQuaternion(config.centerOfMass, state.rotation));
}

Vector3 GetWheelForward(const VehicleState& state, int wheelIndex) {
    Quaternion steerRotation = QuaternionFromAxisAngle(
        Vector3{0.0f, 1.0f, 0.0f},
        state.wheelSteerAngles[wheelIndex]);
    Vector3 localForward = Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, steerRotation);
    return Vector3Normalize(LocalDirectionToWorld(state, localForward));
}

Vector3 GetWheelRight(const VehicleState& state, int wheelIndex) {
    Quaternion steerRotation = QuaternionFromAxisAngle(
        Vector3{0.0f, 1.0f, 0.0f},
        state.wheelSteerAngles[wheelIndex]);
    Vector3 localRight = Vector3RotateByQuaternion(Vector3{1.0f, 0.0f, 0.0f}, steerRotation);
    return Vector3Normalize(LocalDirectionToWorld(state, localRight));
}

Vector3 GetPointVelocity(
    const VehicleState& state,
    const VehicleConfig& config,
    const Vector3& point) {
    Vector3 centerOfMass = GetCenterOfMassWorld(state, config);
    return Vector3Add(
        state.linearVelocity,
        Vector3CrossProduct(state.angularVelocity, Vector3Subtract(point, centerOfMass)));
}

Vector3 GetBoxInertia(const VehicleConfig& config) {
    float width = config.colliderSize.x;
    float height = config.colliderSize.y;
    float length = config.colliderSize.z;
    float sharedFactor = config.mass / 12.0f;

    return Vector3{
        sharedFactor * (height * height + length * length),
        sharedFactor * (width * width + length * length),
        sharedFactor * (width * width + height * height)
    };
}

void ApplyForce(
    VehicleState& state,
    const VehicleConfig& config,
    const Vector3& force,
    const Vector3& worldPoint,
    float deltaTime) {
    state.linearVelocity = Vector3Add(
        state.linearVelocity,
        Vector3Scale(force, deltaTime / config.mass));

    Vector3 centerOfMass = GetCenterOfMassWorld(state, config);
    Vector3 torque = Vector3CrossProduct(Vector3Subtract(worldPoint, centerOfMass), force);
    Vector3 inertia = GetBoxInertia(config);

    Vector3 angularAcceleration = {
        inertia.x > 0.0f ? torque.x / inertia.x : 0.0f,
        inertia.y > 0.0f ? torque.y / inertia.y : 0.0f,
        inertia.z > 0.0f ? torque.z / inertia.z : 0.0f
    };

    state.angularVelocity = Vector3Add(
        state.angularVelocity,
        Vector3Scale(angularAcceleration, deltaTime));
}

void ApplyTorque(
    VehicleState& state,
    const VehicleConfig& config,
    const Vector3& torque,
    float deltaTime) {
    Vector3 inertia = GetBoxInertia(config);
    Vector3 angularAcceleration = {
        inertia.x > 0.0f ? torque.x / inertia.x : 0.0f,
        inertia.y > 0.0f ? torque.y / inertia.y : 0.0f,
        inertia.z > 0.0f ? torque.z / inertia.z : 0.0f
    };

    state.angularVelocity = Vector3Add(
        state.angularVelocity,
        Vector3Scale(angularAcceleration, deltaTime));
}

void IntegrateOrientation(VehicleState& state, float deltaTime) {
    float angularSpeed = Vector3Length(state.angularVelocity);
    if (angularSpeed <= 0.0001f) {
        return;
    }

    Quaternion deltaRotation = QuaternionFromAxisAngle(
        Vector3Scale(state.angularVelocity, 1.0f / angularSpeed),
        angularSpeed * deltaTime);
    state.rotation = QuaternionNormalize(QuaternionMultiply(deltaRotation, state.rotation));
}

float GetHorizontalHalfExtent(const VehicleState& state, const VehicleConfig& config) {
    Vector3 right = GetVehicleRight(state);
    Vector3 up = GetVehicleUp(state);
    Vector3 forward = GetVehicleForward(state);
    Vector3 halfExtents = Vector3Scale(config.colliderSize, 0.5f);

    return fabsf(right.x) * halfExtents.x +
           fabsf(up.x) * halfExtents.y +
           fabsf(forward.x) * halfExtents.z;
}

}  // namespace

VehicleConfig DefaultVehicleConfig() {
    VehicleConfig config = {};
    config.fixedTimeStep = 1.0f / 240.0f;
    config.mass = 50.0f;
    config.enginePower = 18.0f;
    config.downhillMultiplier = 1.3f;
    config.brakePower = 25.0f;
    config.tireTurnSpeed = 1.3f;
    config.tireMaxTurnDegrees = 35.0f;
    config.gripPower = 10.0f;
    config.gripFront = 0.9f;
    config.gripRear = 1.0f;
    config.gripDriftFront = 0.9f;
    config.gripDriftRear = 0.9f;
    config.airPitchTorque = 0.2f;
    config.extraGravity = 12.0f;
    config.wallPenaltyMultiplier = 0.8f;
    config.wallSpinDamping = 0.5f;
    config.groundLinearDamp = 0.1f;
    config.colliderSize = Vector3{2.0f, 0.5f, 4.0f};
    config.centerOfMass = Vector3Zero();
    config.wheelOffsets = {
        Vector3{-0.75f, 0.0f, -1.25f},
        Vector3{0.75f, 0.0f, -1.25f},
        Vector3{-0.75f, 0.0f, 1.25f},
        Vector3{0.75f, 0.0f, 1.25f}
    };
    config.maxTurnCurve = {
        CurvePoint{0.0f, 1.0f},
        CurvePoint{50.0f, 0.4f},
        CurvePoint{100.0f, 0.2f},
        CurvePoint{200.0f, 0.1f},
        CurvePoint{300.0f, 0.05f}
    };
    config.dragCurve = {
        CurvePoint{0.0f, 0.0f},
        CurvePoint{200.0f, 5.0f}
    };
    return config;
}

VehicleConfig LoadVehicleConfig(
    const std::string& filePath,
    const VehicleConfig& fallbackConfig) {
    VehicleConfig config = fallbackConfig;

    char* rawFileContents = LoadFileText(filePath.c_str());
    if (rawFileContents == nullptr) {
        return config;
    }

    std::string json = rawFileContents;
    UnloadFileText(rawFileContents);

    config.fixedTimeStep = ExtractFloat(json, "fixed_time_step", fallbackConfig.fixedTimeStep);
    config.mass = ExtractFloat(json, "mass", fallbackConfig.mass);
    config.enginePower = ExtractFloat(json, "engine_power", fallbackConfig.enginePower);
    config.downhillMultiplier = ExtractFloat(
        json, "downhill_multiplier", fallbackConfig.downhillMultiplier);
    config.brakePower = ExtractFloat(json, "brake_power", fallbackConfig.brakePower);
    config.tireTurnSpeed = ExtractFloat(json, "tire_turn_speed", fallbackConfig.tireTurnSpeed);
    config.tireMaxTurnDegrees = ExtractFloat(
        json, "tire_max_turn_degrees", fallbackConfig.tireMaxTurnDegrees);
    config.gripPower = ExtractFloat(json, "grip_power", fallbackConfig.gripPower);
    config.gripFront = ExtractFloat(json, "grip_front", fallbackConfig.gripFront);
    config.gripRear = ExtractFloat(json, "grip_rear", fallbackConfig.gripRear);
    config.gripDriftFront = ExtractFloat(
        json, "grip_drift_front", fallbackConfig.gripDriftFront);
    config.gripDriftRear = ExtractFloat(
        json, "grip_drift_rear", fallbackConfig.gripDriftRear);
    config.airPitchTorque = ExtractFloat(
        json, "air_pitch_torque", fallbackConfig.airPitchTorque);
    config.extraGravity = ExtractFloat(json, "extra_gravity", fallbackConfig.extraGravity);
    config.wallPenaltyMultiplier = ExtractFloat(
        json, "wall_penalty_multiplier", fallbackConfig.wallPenaltyMultiplier);
    config.wallSpinDamping = ExtractFloat(
        json, "wall_spin_damping", fallbackConfig.wallSpinDamping);
    config.groundLinearDamp = ExtractFloat(
        json, "ground_linear_damp", fallbackConfig.groundLinearDamp);
    config.colliderSize = ExtractVector3(json, "collider_size", fallbackConfig.colliderSize);
    config.centerOfMass = ExtractVector3(json, "center_of_mass", fallbackConfig.centerOfMass);
    config.wheelOffsets = ExtractWheelOffsets(json, "wheel_offsets", fallbackConfig.wheelOffsets);
    config.maxTurnCurve = ExtractCurve(json, "max_turn_curve", fallbackConfig.maxTurnCurve);
    config.dragCurve = ExtractCurve(json, "drag_curve", fallbackConfig.dragCurve);

    return config;
}

VehicleState CreateVehicleState(const VehicleConfig& config) {
    VehicleState state = {};
    ResetVehicleState(state, config);
    return state;
}

void ResetVehicleState(VehicleState& state, const VehicleConfig& config) {
    state.position = Vector3{0.0f, config.colliderSize.y * 0.5f, 0.0f};
    state.rotation = QuaternionIdentity();
    state.linearVelocity = Vector3Zero();
    state.angularVelocity = Vector3Zero();
    state.wheelSteerAngles = {0.0f, 0.0f, 0.0f, 0.0f};
    state.wheelEngineForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    state.wheelGripForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    state.wheelDragBrakeForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    state.previousLinearVelocity = Vector3Zero();
    state.speedKph = 0.0f;
    state.grounded = true;
    state.drifting = false;
}

void StepVehiclePhysics(
    VehicleState& state,
    const VehicleConfig& config,
    const VehicleInput& input,
    const WorldCollisionConfig& world,
    float deltaTime) {
    state.previousLinearVelocity = state.linearVelocity;
    state.wheelEngineForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    state.wheelGripForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};
    state.wheelDragBrakeForces = {Vector3Zero(), Vector3Zero(), Vector3Zero(), Vector3Zero()};

    float halfHeight = config.colliderSize.y * 0.5f;
    state.grounded = state.position.y <= world.groundY + halfHeight + 0.02f;
    if (state.grounded && state.position.y < world.groundY + halfHeight) {
        state.position.y = world.groundY + halfHeight;
        if (state.linearVelocity.y < 0.0f) {
            state.linearVelocity.y = 0.0f;
        }
    }

    state.speedKph = Vector3DotProduct(GetVehicleForward(state), state.linearVelocity) * 3.6f;

    float maxSlipAngleNorm = 0.0f;
    float carMassShare = config.mass / static_cast<float>(config.wheelOffsets.size());
    for (int i = 0; i < static_cast<int>(config.wheelOffsets.size()); ++i) {
        bool isFrontWheel = i < 2;
        bool isPoweredWheel = i >= 2;
        float steerInput = input.steering * config.tireTurnSpeed;

        if (isFrontWheel) {
            float steerRatio = SampleCurve(config.maxTurnCurve, fabsf(state.speedKph));
            float maxTurnRadians = DEG2RAD * config.tireMaxTurnDegrees * steerRatio;
            if (fabsf(steerInput) > 0.0f) {
                state.wheelSteerAngles[i] = Clamp(
                    state.wheelSteerAngles[i] + steerInput * deltaTime,
                    -maxTurnRadians,
                    maxTurnRadians);
            } else {
                state.wheelSteerAngles[i] = Lerp(
                    state.wheelSteerAngles[i],
                    0.0f,
                    Clamp(config.tireTurnSpeed * deltaTime, 0.0f, 1.0f));
            }
        } else {
            state.wheelSteerAngles[i] = 0.0f;
        }

        if (!state.grounded) {
            continue;
        }

        Vector3 wheelCenter = GetWheelWorldPosition(state, config, i);
        Vector3 wheelForward = GetWheelForward(state, i);
        Vector3 wheelRight = GetWheelRight(state, i);
        Vector3 tireVelocity = GetPointVelocity(state, config, wheelCenter);
        float wheelForwardVelocity = Vector3DotProduct(wheelForward, tireVelocity);

        if (isPoweredWheel && input.throttle > 0.0f) {
            Vector3 engineForce = Vector3Scale(
                wheelForward,
                input.throttle * config.mass * config.enginePower * 0.5f);
            if (state.linearVelocity.y < -1.0f) {
                engineForce = Vector3Scale(engineForce, config.downhillMultiplier);
            }
            state.wheelEngineForces[i] = engineForce;
            ApplyForce(state, config, engineForce, wheelCenter, deltaTime);
        }

        float wheelSidewaysVelocity = Vector3DotProduct(wheelRight, tireVelocity);
        float slipAngle = atan2f(wheelSidewaysVelocity, wheelForwardVelocity);
        float slipAngleNorm = Clamp(fabsf(slipAngle) / (PI * 0.5f), 0.0f, 1.0f);
        maxSlipAngleNorm = fmaxf(maxSlipAngleNorm, slipAngleNorm);

        float gripFactor = isFrontWheel ? config.gripFront : config.gripRear;
        if (state.drifting) {
            gripFactor = isFrontWheel ? config.gripDriftFront : config.gripDriftRear;
        }

        Vector3 gripForce = Vector3Scale(
            wheelRight,
            -wheelSidewaysVelocity * carMassShare * config.gripPower * gripFactor);
        state.wheelGripForces[i] = gripForce;
        ApplyForce(state, config, gripForce, wheelCenter, deltaTime);

        Vector3 forceBasis = Vector3Scale(wheelForward, -carMassShare);
        float drag = SampleCurve(config.dragCurve, fabsf(state.speedKph));
        Vector3 dragForce = Vector3Scale(
            forceBasis,
            drag * (2.0f - input.throttle) * SignNonZero(state.speedKph));

        float brakeModifier = state.speedKph < 1.0f ? 0.4f : 1.0f;
        float brakeInput = input.brake;
        Vector3 brakingForce = Vector3Scale(
            forceBasis,
            config.brakePower * brakeModifier * brakeInput);
        state.wheelDragBrakeForces[i] = Vector3Add(dragForce, brakingForce);
        ApplyForce(
            state,
            config,
            state.wheelDragBrakeForces[i],
            wheelCenter,
            deltaTime);
    }

    if (input.brake > 0.0f) {
        state.drifting = true;
    } else if (maxSlipAngleNorm < 0.05f) {
        state.drifting = false;
    }

    if (!state.grounded) {
        Vector3 pitchTorque = Vector3Scale(GetVehicleRight(state), -config.airPitchTorque * config.mass);
        ApplyTorque(state, config, pitchTorque, deltaTime);
        state.linearVelocity.y -= (kBaseGravity + config.extraGravity) * deltaTime;
    } else {
        float dampFactor = 1.0f / (1.0f + config.groundLinearDamp * deltaTime);
        state.linearVelocity = Vector3Scale(state.linearVelocity, dampFactor);
    }

    state.position = Vector3Add(state.position, Vector3Scale(state.linearVelocity, deltaTime));
    IntegrateOrientation(state, deltaTime);

    if (state.position.y <= world.groundY + halfHeight) {
        state.position.y = world.groundY + halfHeight;
        if (state.linearVelocity.y < 0.0f) {
            state.linearVelocity.y = 0.0f;
        }
        state.grounded = true;
    } else {
        state.grounded = false;
    }

    float halfExtentX = GetHorizontalHalfExtent(state, config);
    bool hitWall = false;
    Vector3 wallNormal = Vector3Zero();
    if (state.position.x + halfExtentX > world.roadHalfWidth) {
        state.position.x = world.roadHalfWidth - halfExtentX;
        wallNormal = Vector3{-1.0f, 0.0f, 0.0f};
        hitWall = true;
    } else if (state.position.x - halfExtentX < -world.roadHalfWidth) {
        state.position.x = -world.roadHalfWidth + halfExtentX;
        wallNormal = Vector3{1.0f, 0.0f, 0.0f};
        hitWall = true;
    }

    if (hitWall) {
        float impactVelocity = fabsf(Vector3DotProduct(state.previousLinearVelocity, wallNormal));
        if (impactVelocity > 0.5f) {
            Vector3 carForward = GetVehicleForward(state);
            float currentForwardSpeed = Vector3DotProduct(state.linearVelocity, carForward);
            float speedReduction = impactVelocity * config.wallPenaltyMultiplier;
            float newForwardSpeed = MoveTowardsScalar(currentForwardSpeed, 0.0f, speedReduction);
            float actualReduction = currentForwardSpeed - newForwardSpeed;
            state.linearVelocity = Vector3Subtract(
                state.linearVelocity,
                Vector3Scale(carForward, actualReduction));
            state.angularVelocity = Vector3Scale(state.angularVelocity, config.wallSpinDamping);
        }
    }

    state.speedKph = Vector3DotProduct(GetVehicleForward(state), state.linearVelocity) * 3.6f;
}

Vector3 GetVehicleForward(const VehicleState& state) {
    return Vector3Normalize(LocalDirectionToWorld(state, Vector3{0.0f, 0.0f, -1.0f}));
}

Vector3 GetVehicleRight(const VehicleState& state) {
    return Vector3Normalize(LocalDirectionToWorld(state, Vector3{1.0f, 0.0f, 0.0f}));
}

Vector3 GetVehicleUp(const VehicleState& state) {
    return Vector3Normalize(LocalDirectionToWorld(state, Vector3{0.0f, 1.0f, 0.0f}));
}

Vector3 GetWheelWorldPosition(const VehicleState& state, const VehicleConfig& config, int wheelIndex) {
    return Vector3Add(
        state.position,
        Vector3RotateByQuaternion(config.wheelOffsets[wheelIndex], state.rotation));
}

float GetVehicleYawDegrees(const VehicleState& state) {
    Vector3 forward = GetVehicleForward(state);
    return RAD2DEG * atan2f(-forward.x, -forward.z);
}

float GetAverageFrontSteerDegrees(const VehicleState& state) {
    return RAD2DEG * ((state.wheelSteerAngles[0] + state.wheelSteerAngles[1]) * 0.5f);
}
