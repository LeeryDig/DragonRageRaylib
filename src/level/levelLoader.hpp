#ifndef LEVEL_LEVEL_LOADER_HPP
#define LEVEL_LEVEL_LOADER_HPP

#include <string>

#include "levelData.hpp"

LevelData LoadLevel(const std::string& levelPath);
void UnloadLevel(LevelData& level);
void DrawLevel(const LevelData& level);
void ApplyLevelRootTransform(LevelData& level, Vector3 position, Quaternion rotation);
void ApplyLevelDebugNodeTransform(LevelData& level, int debugNodeIndex, Vector3 position, Quaternion rotation, Vector3 scale);
void TeleportLevelDebugNodeToCamera(LevelData& level, int debugNodeIndex, const Camera& camera);
void DrawLevelDebugSelection(const LevelData& level, int debugNodeIndex, const Camera& camera);
const char* LevelDebugNodeKindName(LevelDebugNodeKind kind);

#endif
