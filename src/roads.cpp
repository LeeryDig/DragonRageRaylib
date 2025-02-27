#include "roads.hpp"

std::vector<Roads> GenerateRoads(int num, float beginPosition) {
    std::vector<Roads> roads;

    for (size_t i = 0; i < num; i++) {
        Model model = LoadModel("resources/models/RoadAsphalt.glb");
        Texture2D roadTexture = LoadTexture("resources/models/RoadAsphaltDiff.jpg");
        SetMaterialTexture(&model.materials[0], MATERIAL_MAP_DIFFUSE, roadTexture);
        BoundingBox bbox = GetModelBoundingBox(model);

        Roads road;
        road.model = model;
        road.position = (Vector3){0.0, 0.001, beginPosition};
        road.length = bbox.max.z - bbox.min.z;
        beginPosition += road.length;
        roads.push_back(road);
    }
    return roads;
}

void DrawRoads(std::vector<Roads>& roads) {
    for (const auto& road : roads) {
        DrawModelEx(road.model, road.position, (Vector3){0.0, 1.0, 0.0}, 0.0f, (Vector3){1.0, 1.0, 1.0}, WHITE);
    }
}

void UpdateRoads(std::vector<Roads>& roads, float speed) {
    for (auto& road : roads) {
        road.position.z += speed;
    }

    for (auto& road : roads) {
        if (road.position.z >= 20.0f) {
            // Encontra a road que está mais para trás (maior valor de Z negativo)
            float lastRoadZ = roads[0].position.z;
            for (const auto& r : roads) {
                if (r.position.z < lastRoadZ) {
                    lastRoadZ = r.position.z;
                }
            }

            // Move a road para trás da última road
            road.position.z = lastRoadZ - road.length;
        }
    }
}