#ifndef CAMERA_DEBUG_HPP
#define CAMERA_DEBUG_HPP

#include <string>

#include "raylib.h"

struct CockpitCameraConfig {
    Vector3 positionOffset;
    Vector3 targetOffset;
    float fovy;
};

struct ChaseCameraConfig {
    float distance;
    float height;
    float verticalStiffness;
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

ChaseCameraConfig LoadChaseCameraConfig(
    const std::string& filePath,
    const ChaseCameraConfig& fallbackConfig);

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

Camera CreateChaseCamera(
    const Vector3& anchorPosition,
    const Quaternion& anchorRotation,
    const ChaseCameraConfig& config);

void ApplyChaseCamera(
    Camera& camera,
    const Vector3& anchorPosition,
    const Quaternion& anchorRotation,
    const ChaseCameraConfig& config,
    float deltaTime);

void SyncDebugCameraRotation(DebugCameraState& debugCamera, const Camera& camera);

void UpdateDebugCamera(DebugCameraState& debugCamera, Camera& camera);

#endif
