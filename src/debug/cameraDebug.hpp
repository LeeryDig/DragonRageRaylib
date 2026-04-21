#ifndef CAMERA_DEBUG_HPP
#define CAMERA_DEBUG_HPP

#include <string>

#include "raylib.h"

struct CockpitCameraConfig {
    Vector3 positionOffset;
    Vector3 targetOffset;
    float fovy;
};

struct DebugCameraState {
    bool enabled;
    float moveSpeed;
    float lookSensitivity;
    float yaw;
    float pitch;
};

CockpitCameraConfig LoadCockpitCameraConfig(
    const std::string& filePath,
    const CockpitCameraConfig& fallbackConfig);

DebugCameraState LoadDebugCameraStateConfig(
    const std::string& filePath,
    const DebugCameraState& fallbackState);

Camera CreateCockpitCamera(
    const Vector3& anchorPosition,
    const Quaternion& anchorRotation,
    const CockpitCameraConfig& config);

void ApplyCockpitCamera(
    Camera& camera,
    const Vector3& anchorPosition,
    const Quaternion& anchorRotation,
    const CockpitCameraConfig& config);

void SyncDebugCameraRotation(DebugCameraState& debugCamera, const Camera& camera);

void UpdateDebugCamera(DebugCameraState& debugCamera, Camera& camera);

#endif
