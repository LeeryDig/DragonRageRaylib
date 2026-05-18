#ifndef DEBUG_LEVEL_DEBUG_DRAW_HPP
#define DEBUG_LEVEL_DEBUG_DRAW_HPP

#include "raylib.h"

#include "level/levelData.hpp"
#include "personController.hpp"

void DrawDebugLevelBoxWire(const LevelBoxVolume& box, Color color);
void DrawLevelCollidersDebug(const LevelData& level);
void DrawPersonDebugCapsule(const PersonState& person, const PersonConfig& config);
void DrawDebugAxisGizmo(const Camera& camera);

#endif
