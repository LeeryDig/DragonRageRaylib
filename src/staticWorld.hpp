#ifndef STATIC_WORLD_HPP
#define STATIC_WORLD_HPP

#include <vector>

#include "raylib.h"

struct StaticWorld {
    Model buildingModel;
    Texture2D buildingTexture;
    std::vector<Vector3> buildingPositions;
    float groundY;
    float roadHalfWidth;
    float roadLength;
    float roadCenterZ;
    float roadThickness;
    float buildingScale;
};

StaticWorld LoadStaticWorld();
void UnloadStaticWorld(StaticWorld& world);
void DrawStaticWorld(const StaticWorld& world);
Vector3 GetRoadColliderCenter(const StaticWorld& world);
Vector3 GetRoadColliderSize(const StaticWorld& world);

#endif
