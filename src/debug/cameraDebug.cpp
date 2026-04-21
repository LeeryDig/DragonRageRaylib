#include "cameraDebug.hpp"

#include <cstdio>
#include <string>

#include "raymath.h"

namespace {

Vector3 ExtractVector3(const std::string& json, const std::string& key, Vector3 fallbackValue) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return fallbackValue;
    }

    std::size_t arrayStart = json.find('[', keyPos);
    if (arrayStart == std::string::npos) {
        return fallbackValue;
    }

    float x = fallbackValue.x;
    float y = fallbackValue.y;
    float z = fallbackValue.z;
    if (std::sscanf(json.c_str() + arrayStart, "[ %f , %f , %f ]", &x, &y, &z) == 3) {
        return Vector3{x, y, z};
    }

    return fallbackValue;
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

    float parsedValue = fallbackValue;
    if (std::sscanf(json.c_str() + valueStart + 1, " %f", &parsedValue) == 1) {
        return parsedValue;
    }

    return fallbackValue;
}

Vector3 BuildForwardVector(float yaw, float pitch) {
    return Vector3Normalize(Vector3{
        sinf(yaw) * cosf(pitch),
        sinf(pitch),
        -cosf(yaw) * cosf(pitch)
    });
}

}  // namespace

CockpitCameraConfig LoadCockpitCameraConfig(
    const std::string& filePath,
    const CockpitCameraConfig& fallbackConfig) {
    CockpitCameraConfig loadedConfig = fallbackConfig;

    char* rawFileContents = LoadFileText(filePath.c_str());
    if (rawFileContents == nullptr) {
        return loadedConfig;
    }

    std::string json = rawFileContents;
    UnloadFileText(rawFileContents);

    loadedConfig.positionOffset = ExtractVector3(json, "position", fallbackConfig.positionOffset);
    loadedConfig.targetOffset = ExtractVector3(json, "target", fallbackConfig.targetOffset);
    loadedConfig.fovy = ExtractFloat(json, "fovy", fallbackConfig.fovy);

    return loadedConfig;
}

DebugCameraState LoadDebugCameraStateConfig(
    const std::string& filePath,
    const DebugCameraState& fallbackState) {
    DebugCameraState loadedState = fallbackState;

    char* rawFileContents = LoadFileText(filePath.c_str());
    if (rawFileContents == nullptr) {
        return loadedState;
    }

    std::string json = rawFileContents;
    UnloadFileText(rawFileContents);

    loadedState.moveSpeed = ExtractFloat(json, "moveSpeed", fallbackState.moveSpeed);
    loadedState.lookSensitivity = ExtractFloat(json, "lookSensitivity", fallbackState.lookSensitivity);

    return loadedState;
}

Camera CreateCockpitCamera(
    const Vector3& anchorPosition,
    const Quaternion& anchorRotation,
    const CockpitCameraConfig& config) {
    Camera camera = {};
    camera.position = Vector3Add(
        anchorPosition,
        Vector3RotateByQuaternion(config.positionOffset, anchorRotation));
    camera.target = Vector3Add(
        anchorPosition,
        Vector3RotateByQuaternion(config.targetOffset, anchorRotation));
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = config.fovy;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

void ApplyCockpitCamera(
    Camera& camera,
    const Vector3& anchorPosition,
    const Quaternion& anchorRotation,
    const CockpitCameraConfig& config) {
    camera.position = Vector3Add(
        anchorPosition,
        Vector3RotateByQuaternion(config.positionOffset, anchorRotation));
    camera.target = Vector3Add(
        anchorPosition,
        Vector3RotateByQuaternion(config.targetOffset, anchorRotation));
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = config.fovy;
    camera.projection = CAMERA_PERSPECTIVE;
}

void SyncDebugCameraRotation(DebugCameraState& debugCamera, const Camera& camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    debugCamera.yaw = atan2f(forward.x, -forward.z);
    debugCamera.pitch = asinf(Clamp(forward.y, -1.0f, 1.0f));
}

void UpdateDebugCamera(DebugCameraState& debugCamera, Camera& camera) {
    Vector2 mouseDelta = GetMouseDelta();
    debugCamera.yaw += mouseDelta.x * debugCamera.lookSensitivity;
    debugCamera.pitch = Clamp(
        debugCamera.pitch - mouseDelta.y * debugCamera.lookSensitivity,
        -1.4f,
        1.4f);

    Vector3 forward = BuildForwardVector(debugCamera.yaw, debugCamera.pitch);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, Vector3{0.0f, 1.0f, 0.0f}));

    Vector3 movement = Vector3Zero();
    if (IsKeyDown(KEY_W)) {
        movement = Vector3Add(movement, forward);
    }
    if (IsKeyDown(KEY_S)) {
        movement = Vector3Subtract(movement, forward);
    }
    if (IsKeyDown(KEY_D)) {
        movement = Vector3Add(movement, right);
    }
    if (IsKeyDown(KEY_A)) {
        movement = Vector3Subtract(movement, right);
    }
    if (IsKeyDown(KEY_Q)) {
        movement.y += 1.0f;
    }
    if (IsKeyDown(KEY_Z)) {
        movement.y -= 1.0f;
    }

    if (Vector3LengthSqr(movement) > 0.0f) {
        float moveSpeed = debugCamera.moveSpeed * GetFrameTime();
        movement = Vector3Scale(Vector3Normalize(movement), moveSpeed);
        camera.position = Vector3Add(camera.position, movement);
    }

    camera.target = Vector3Add(camera.position, forward);
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
}
