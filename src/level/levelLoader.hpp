#ifndef LEVEL_LEVEL_LOADER_HPP
#define LEVEL_LEVEL_LOADER_HPP

#include <string>

#include "levelData.hpp"

LevelData LoadLevel(const std::string& levelPath);
void UnloadLevel(LevelData& level);
void DrawLevel(const LevelData& level);

#endif
