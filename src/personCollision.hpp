#ifndef PERSON_COLLISION_HPP
#define PERSON_COLLISION_HPP

#include "level/levelData.hpp"
#include "personController.hpp"

void ResolvePersonLevelCollisions(const LevelData& level, PersonState& person, const PersonConfig& config);
void ApplyPersonGroundSnap(const LevelData& level, PersonState& person, const PersonConfig& config);

#endif
