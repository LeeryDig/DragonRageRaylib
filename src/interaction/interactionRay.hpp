#ifndef INTERACTION_INTERACTION_RAY_HPP
#define INTERACTION_INTERACTION_RAY_HPP

#include "raylib.h"
#include "interactionSystem.hpp"

Ray BuildInteractionRay(const Camera& camera);
void DrawInteractionRayDebug(const Camera& camera, float length);
bool RayHitsInteractionBox(Ray ray, BoundingBox box, float maxDistance, float& hitDistance);
bool RayHitsInteractionCapsule(Ray ray, const CharacterCapsule& capsule, float maxDistance, float aimTolerance, float& hitDistance);

#endif
