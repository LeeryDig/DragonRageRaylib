#ifndef GAMEPLAY_WORLD_ACTIONS_HPP
#define GAMEPLAY_WORLD_ACTIONS_HPP

#include "game/gameWorld.hpp"
#include "level/levelsConfig.hpp"

void ApplyRuntimeRenderConfig(GameWorld& gameWorld);
void ReloadJoltLevelPhysics(GameWorld& gameWorld);

void ResetGameWorld(GameWorld& gameWorld);
void RestartLevel(GameWorld& gameWorld);
void LoadConfiguredLevel(GameWorld& gameWorld, int levelIndex);
void SaveLevelsConfigToDisk(const LevelsConfig& config);
void MoveLevelConfigEntry(GameWorld& gameWorld, int fromIndex, int toIndex);
void SaveCurrentLevelRuntimeConfig(GameWorld& gameWorld);
void ReloadCurrentLevelForConfig(GameWorld& gameWorld);

GameWorld LoadGameWorld();
void UnloadGameWorld(GameWorld& gameWorld);

#endif
