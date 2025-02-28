#include "scenery.hpp"

std::vector<Scenery> GenerateScenery(int num, float beginDistance, float endDistance, float frequency) {
    std::vector<Scenery> listOfScenery;

    for (size_t i = 0; i < num; i++) {
        Model model = LoadModel("resources/models/Building.glb");
        Texture2D objectTexture = LoadTexture("resources/models/Building.png");
        SetMaterialTexture(&model.materials[0], MATERIAL_MAP_DIFFUSE, objectTexture);
        BoundingBox bbox = GetModelBoundingBox(model);

        Scenery scenery;
        scenery.model = model;
        scenery.beginDistance = beginDistance;
        scenery.endDistance = endDistance;
        scenery.frenquecy = frequency;
        scenery.position = (Vector3){0.0, 0.0, -30.0};
        listOfScenery.push_back(scenery);
    }
    return listOfScenery;
}

void DrawScenery(std::vector<Scenery>& listOfScenery, float distance) {
    Scenery firstObject = listOfScenery[0];
    if (distance > firstObject.beginDistance && distance < firstObject.endDistance) {
        for (const auto& object : listOfScenery) {
            DrawModelEx(object.model, object.position, (Vector3){0.0, 1.0, 0.0}, 0.0f, (Vector3){1.0, 1.0, 1.0}, WHITE);
        }
    }
}

void UpdateScenery(std::vector<Scenery>& listOfScenery, float speed) {
    for (auto& object : listOfScenery) {
        object.position.z += speed;
    }

    for (auto& object : listOfScenery) {
        if (object.position.z >= 20.0f) {
            float lastObjectZ = listOfScenery[0].position.z;
            for (const auto& o : listOfScenery) {
                if (o.position.z < lastObjectZ) {
                    lastObjectZ = o.position.z;
                }
            }
            object.position.z = lastObjectZ + object.frenquecy;
        }
    }
}