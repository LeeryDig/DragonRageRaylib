#ifndef PERSON_CONTROLLER_HPP
#define PERSON_CONTROLLER_HPP

#include <string>

#include "raylib.h"

struct PersonConfig {
    float fixedTimeStep;
    float walkSpeed;
    float acceleration;
    float deceleration;
    float turnSpeed;
    float gravity;
    float groundSnapDistance;
    float capsuleRadius;
    float capsuleHeight;
    float eyeHeight;
    float cameraSmooth;
    float cameraMouseSensitivity;
    float interactionDistance;
    float interactionRayLength;
    float cameraPitchMinDegrees;
    float cameraPitchMaxDegrees;
};

struct PersonInput {
    float moveX;
    float moveZ;
};

struct PersonState {
    Vector3 position;
    Vector3 velocity;
    Vector3 forward;
    bool grounded;
    float cameraYaw;
    float cameraPitch;
};

PersonConfig DefaultPersonConfig();

PersonConfig LoadPersonConfig(
    const std::string& filePath,
    const PersonConfig& fallbackConfig);

PersonState CreatePersonState(const PersonConfig& config);

void ResetPersonState(PersonState& state, const PersonConfig& config, Vector3 position, float yawRadians = 0.0f);

PersonInput ReadPersonInput(bool controlsEnabled);

Vector3 GetPersonCameraForward(const PersonState& state);
Vector3 GetPersonCameraRight(const PersonState& state);

void UpdatePersonLook(PersonState& state, const PersonConfig& config);
void UpdatePersonHorizontalMovement(PersonState& state, const PersonConfig& config, const PersonInput& input, float deltaTime);
void ApplyPersonCamera(Camera& camera, const PersonState& state, const PersonConfig& config, float deltaTime);

#endif
