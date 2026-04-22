#include "roads.hpp"
#include "utils.hpp"

std::vector<Roads> GenerateRoads(int total) {
    std::vector<Roads> roads;

    std::string modelPath = Utils::ResolveProjectPath("resources/models/RoadAsphalt.glb");
    std::string texturePath = Utils::ResolveProjectPath("resources/textures/ground/sand_3.png");
    Model model = LoadModel(modelPath.c_str());
    Texture2D roadTexture = LoadTexture(texturePath.c_str());
    SetMaterialTexture(&model.materials[0], MATERIAL_MAP_DIFFUSE, roadTexture);
    BoundingBox bbox = GetModelBoundingBox(model);
    float zPos = -30.0;
    float length = bbox.max.z - bbox.min.z;

    for (std::size_t i = 0; i < total; i++) {
        Roads road;
        road.model = model;
        road.position = (Vector3){0.0, 0.001, zPos};
        road.length = length;
        zPos += road.length;
        roads.push_back(road);
    }
    return roads;
}

void DrawRoads(const std::vector<Roads>& roads) {
    for (const auto& road : roads) {
        DrawModel(road.model, road.position, 1.0, WHITE);
    }
}

void UpdateRoads(std::vector<Roads>& roads, float speed) {
    for (auto& road : roads) {
        road.position.z += (speed * 5) * GetFrameTime();
    }

    for (auto& road : roads) {
        if (road.position.z >= 20.0f) {

            float lastRoadZ = roads[0].position.z;
            for (const auto& r : roads) {
                if (r.position.z < lastRoadZ) {
                    lastRoadZ = r.position.z;
                }
            }

            road.position.z = lastRoadZ - road.length;
        }
    }
}
