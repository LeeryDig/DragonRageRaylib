#include "staticWorld.hpp"

#include <cstddef>

StaticWorld LoadStaticWorld() {
    StaticWorld world = {};
    world.groundY = 0.0f;
    world.roadHalfWidth = 6.0f;
    world.roadLength = 3200.0f;
    world.roadCenterZ = 1500.0f;
    world.roadThickness = 0.08f;
    world.buildingScale = 1.0f;
    world.buildingModel = LoadModel("resources/models/Building.glb");
    world.buildingTexture = LoadTexture("resources/textures/Building.png");
    SetMaterialTexture(&world.buildingModel.materials[0], MATERIAL_MAP_DIFFUSE, world.buildingTexture);

    float startZ = -40.0f;
    float spacing = 28.0f;
    for (std::size_t i = 0; i < 110; ++i) {
        float z = startZ + spacing * static_cast<float>(i);
        world.buildingPositions.push_back(Vector3{-13.0f, 0.0f, z});
        world.buildingPositions.push_back(Vector3{13.0f, 0.0f, z});
    }

    return world;
}

void UnloadStaticWorld(StaticWorld& world) {
    UnloadTexture(world.buildingTexture);
    UnloadModel(world.buildingModel);
}

void DrawStaticWorld(const StaticWorld& world) {
    DrawCube(
        Vector3{0.0f, -0.6f, world.roadCenterZ},
        120.0f,
        1.2f,
        world.roadLength + 200.0f,
        DARKGREEN);

    DrawCube(
        Vector3{0.0f, world.groundY + 0.1f, world.roadCenterZ},
        world.roadHalfWidth * 2.0f,
        world.roadThickness,
        world.roadLength,
        GRAY);

    for (int i = 0; i < 100; ++i) {
        float z = static_cast<float>(i) * 28.0f;
        DrawCube(
            Vector3{0.0f, world.groundY + 0.15f, z},
            0.3f,
            0.01f,
            10.0f,
            RAYWHITE);
    }

    for (const Vector3& position : world.buildingPositions) {
        DrawModel(world.buildingModel, position, world.buildingScale, WHITE);
    }
}

Vector3 GetRoadColliderCenter(const StaticWorld& world) {
    return Vector3{0.0f, world.groundY + 0.1f, world.roadCenterZ};
}

Vector3 GetRoadColliderSize(const StaticWorld& world) {
    return Vector3{
        world.roadHalfWidth * 2.0f,
        world.roadThickness,
        world.roadLength
    };
}
