#include "personController.hpp"

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "raymath.h"

namespace {

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

float MoveTowardsScalar(float value, float target, float maxDelta) {
    if (fabsf(target - value) <= maxDelta) {
        return target;
    }
    return value + (target > value ? maxDelta : -maxDelta);
}

Vector3 MoveTowardsVector3(Vector3 value, Vector3 target, float maxDelta) {
    Vector3 delta = Vector3Subtract(target, value);
    float distance = Vector3Length(delta);
    if (distance <= maxDelta || distance <= 0.0001f) {
        return target;
    }
    return Vector3Add(value, Vector3Scale(delta, maxDelta / distance));
}

}  // namespace

PersonConfig DefaultPersonConfig() {
    PersonConfig config = {};
    config.fixedTimeStep = 1.0f / 120.0f;
    config.walkSpeed = 3.0f;
    config.acceleration = 18.0f;
    config.deceleration = 22.0f;
    config.turnSpeed = 12.0f;
    config.gravity = 9.81f;
    config.groundSnapDistance = 0.25f;
    config.capsuleRadius = 0.3f;
    config.capsuleHeight = 1.2f;
    config.eyeHeight = 1.65f;
    config.cameraSmooth = 0.0f;
    config.cameraMouseSensitivity = 0.003f;
    config.interactionDistance = 2.5f;
    config.interactionRayLength = 4.0f;
    config.cameraPitchMinDegrees = -35.0f;
    config.cameraPitchMaxDegrees = 55.0f;
    return config;
}

PersonConfig LoadPersonConfig(const std::string& filePath, const PersonConfig& fallbackConfig) {
    PersonConfig config = fallbackConfig;

    char* rawFileContents = LoadFileText(filePath.c_str());
    if (rawFileContents == nullptr) {
        return config;
    }

    std::string json = rawFileContents;
    UnloadFileText(rawFileContents);

    config.fixedTimeStep = ExtractFloat(json, "fixed_time_step", fallbackConfig.fixedTimeStep);
    config.walkSpeed = ExtractFloat(json, "walk_speed", fallbackConfig.walkSpeed);
    config.acceleration = ExtractFloat(json, "acceleration", fallbackConfig.acceleration);
    config.deceleration = ExtractFloat(json, "deceleration", fallbackConfig.deceleration);
    config.turnSpeed = ExtractFloat(json, "turn_speed", fallbackConfig.turnSpeed);
    config.gravity = ExtractFloat(json, "gravity", fallbackConfig.gravity);
    config.groundSnapDistance = ExtractFloat(json, "ground_snap_distance", fallbackConfig.groundSnapDistance);
    config.capsuleRadius = ExtractFloat(json, "capsule_radius", fallbackConfig.capsuleRadius);
    config.capsuleHeight = ExtractFloat(json, "capsule_height", fallbackConfig.capsuleHeight);
    config.eyeHeight = ExtractFloat(json, "eye_height", fallbackConfig.eyeHeight);
    config.cameraSmooth = ExtractFloat(json, "camera_smooth", fallbackConfig.cameraSmooth);
    config.cameraMouseSensitivity = ExtractFloat(json, "camera_mouse_sensitivity", fallbackConfig.cameraMouseSensitivity);
    config.interactionDistance = ExtractFloat(json, "interaction_distance", fallbackConfig.interactionDistance);
    config.interactionRayLength = ExtractFloat(json, "interaction_ray_length", fallbackConfig.interactionRayLength);
    config.cameraPitchMinDegrees = ExtractFloat(json, "camera_pitch_min_degrees", fallbackConfig.cameraPitchMinDegrees);
    config.cameraPitchMaxDegrees = ExtractFloat(json, "camera_pitch_max_degrees", fallbackConfig.cameraPitchMaxDegrees);

    return config;
}

PersonState CreatePersonState(const PersonConfig& config) {
    PersonState state = {};
    ResetPersonState(state, config, Vector3{0.0f, 2.0f, 0.0f});
    return state;
}

void ResetPersonState(PersonState& state, const PersonConfig& config, Vector3 position, float yawRadians) {
    (void)config;
    state.position = position;
    state.velocity = Vector3Zero();
    state.forward = Vector3{0.0f, 0.0f, -1.0f};
    state.grounded = false;
    state.cameraYaw = yawRadians;
    state.cameraPitch = 15.0f * DEG2RAD;
}

PersonInput ReadPersonInput(bool controlsEnabled) {
    PersonInput input = {0.0f, 0.0f};
    if (!controlsEnabled) {
        return input;
    }

    if (IsKeyDown(KEY_A)) input.moveX -= 1.0f;
    if (IsKeyDown(KEY_D)) input.moveX += 1.0f;
    if (IsKeyDown(KEY_W)) input.moveZ += 1.0f;
    if (IsKeyDown(KEY_S)) input.moveZ -= 1.0f;

    Vector2 move = Vector2{input.moveX, input.moveZ};
    if (Vector2LengthSqr(move) > 1.0f) {
        move = Vector2Normalize(move);
        input.moveX = move.x;
        input.moveZ = move.y;
    }

    return input;
}

Vector3 GetPersonCameraForward(const PersonState& state) {
    float cosPitch = cosf(state.cameraPitch);
    return Vector3Normalize(Vector3{
        sinf(state.cameraYaw) * cosPitch,
        sinf(state.cameraPitch),
        -cosf(state.cameraYaw) * cosPitch
    });
}

Vector3 GetPersonCameraRight(const PersonState& state) {
    Vector3 forward = GetPersonCameraForward(state);
    Vector3 right = Vector3CrossProduct(forward, Vector3{0.0f, 1.0f, 0.0f});
    if (Vector3LengthSqr(right) <= 0.0001f) {
        return Vector3{1.0f, 0.0f, 0.0f};
    }
    return Vector3Normalize(right);
}

void UpdatePersonLook(PersonState& state, const PersonConfig& config) {
    Vector2 mouseDelta = GetMouseDelta();
    state.cameraYaw += mouseDelta.x * config.cameraMouseSensitivity;
    state.cameraPitch -= mouseDelta.y * config.cameraMouseSensitivity;
    state.cameraPitch = Clamp(
        state.cameraPitch,
        config.cameraPitchMinDegrees * DEG2RAD,
        config.cameraPitchMaxDegrees * DEG2RAD);
}

void UpdatePersonHorizontalMovement(PersonState& state, const PersonConfig& config, const PersonInput& input, float deltaTime) {
    Vector3 cameraForward = GetPersonCameraForward(state);
    cameraForward.y = 0.0f;
    if (Vector3LengthSqr(cameraForward) <= 0.0001f) {
        cameraForward = state.forward;
    } else {
        cameraForward = Vector3Normalize(cameraForward);
    }

    Vector3 cameraRight = GetPersonCameraRight(state);
    cameraRight.y = 0.0f;
    if (Vector3LengthSqr(cameraRight) <= 0.0001f) {
        cameraRight = Vector3{1.0f, 0.0f, 0.0f};
    } else {
        cameraRight = Vector3Normalize(cameraRight);
    }

    Vector3 desiredDirection = Vector3Add(
        Vector3Scale(cameraRight, input.moveX),
        Vector3Scale(cameraForward, input.moveZ));
    if (Vector3LengthSqr(desiredDirection) > 0.0001f) {
        desiredDirection = Vector3Normalize(desiredDirection);
    }

    Vector3 horizontalVelocity = Vector3{state.velocity.x, 0.0f, state.velocity.z};
    Vector3 desiredVelocity = Vector3Scale(desiredDirection, config.walkSpeed);
    float rate = Vector3LengthSqr(desiredDirection) > 0.0001f ? config.acceleration : config.deceleration;
    horizontalVelocity = MoveTowardsVector3(horizontalVelocity, desiredVelocity, rate * deltaTime);

    state.velocity.x = horizontalVelocity.x;
    state.velocity.z = horizontalVelocity.z;

    if (Vector3LengthSqr(desiredDirection) > 0.0001f) {
        state.forward = Vector3Normalize(Vector3Lerp(
            state.forward,
            desiredDirection,
            Clamp(config.turnSpeed * deltaTime, 0.0f, 1.0f)));
    }
}

void ApplyPersonCamera(Camera& camera, const PersonState& state, const PersonConfig& config, float deltaTime) {
    Vector3 eyePosition = Vector3Add(state.position, Vector3{0.0f, config.eyeHeight, 0.0f});
    Vector3 forward = GetPersonCameraForward(state);

    if (config.cameraSmooth > 0.0f) {
        float t = Clamp(config.cameraSmooth * deltaTime, 0.0f, 1.0f);
        camera.position = Vector3Lerp(camera.position, eyePosition, t);
        camera.target = Vector3Lerp(camera.target, Vector3Add(eyePosition, forward), t);
    } else {
        camera.position = eyePosition;
        camera.target = Vector3Add(eyePosition, forward);
    }
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}
