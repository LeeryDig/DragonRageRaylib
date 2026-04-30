#ifndef VEHICLE_PHYSICS_HPP
#define VEHICLE_PHYSICS_HPP

#include <array>
#include <string>
#include <vector>

#include "raylib.h"

struct CurvePoint {
    float x;
    float y;
};

struct VehicleConfig {
    float fixedTimeStep;
    float mass;
    float enginePower;
    float downhillMultiplier;
    float brakePower;
    float brakeForceMultiplier;
    float tireTurnSpeed;
    float tireMaxTurnDegrees;
    float gripPower;
    float gripFront;
    float gripRear;
    float gripDriftFront;
    float gripDriftRear;
    float airPitchTorque;
    float extraGravity;
    float wallPenaltyMultiplier;
    float wallSpinDamping;
    float groundLinearDamp;
    float angularDamp;
    float suspensionRestLength;
    float suspensionRayLength;
    float suspensionStrength;
    float suspensionDamping;
    Vector3 colliderSize;
    Vector3 centerOfMass;
    std::array<Vector3, 4> wheelOffsets;
    std::vector<CurvePoint> maxTurnCurve;
    std::vector<CurvePoint> dragCurve;
};

struct VehicleInput {
    float throttle;
    float brake;
    float steering;
};

struct VehicleState {
    Vector3 position;
    Quaternion rotation;
    Vector3 linearVelocity;
    Vector3 angularVelocity;
    std::array<float, 4> wheelSteerAngles;
    std::array<Vector3, 4> wheelEngineForces;
    std::array<Vector3, 4> wheelGripForces;
    std::array<Vector3, 4> wheelDragBrakeForces;
    Vector3 previousLinearVelocity;
    float speedKph;
    bool grounded;
    bool drifting;
};

VehicleConfig DefaultVehicleConfig();

VehicleConfig LoadVehicleConfig(
    const std::string& filePath,
    const VehicleConfig& fallbackConfig);

VehicleState CreateVehicleState(const VehicleConfig& config);

void ResetVehicleState(VehicleState& state, const VehicleConfig& config);

void StepVehiclePhysics(
    VehicleState& state,
    const VehicleConfig& config,
    const VehicleInput& input,
    float deltaTime);

Vector3 GetVehicleForward(const VehicleState& state);
Vector3 GetVehicleRight(const VehicleState& state);
Vector3 GetVehicleUp(const VehicleState& state);
Vector3 GetWheelWorldPosition(const VehicleState& state, const VehicleConfig& config, int wheelIndex);
float GetVehicleYawDegrees(const VehicleState& state);
float GetAverageFrontSteerDegrees(const VehicleState& state);

#endif
