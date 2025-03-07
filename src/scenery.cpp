#include "scenery.hpp"

#include <iostream>

std::vector<Scenery> GenerateScenery(float xPos, float total, float frequency) {
    std::vector<Scenery> listOfScenery;

    Model model = LoadModel("resources/models/Building.glb");
    Texture2D objectTexture = LoadTexture("resources/textures/Building.png");
    SetMaterialTexture(&model.materials[0], MATERIAL_MAP_DIFFUSE, objectTexture);
    BoundingBox bbox = GetModelBoundingBox(model);
    float zPos = -30.0;
    float length = bbox.max.z - bbox.min.z;
    
    for (size_t i = 0; i < total; i++) {
        Scenery scenery;
        scenery.model = model;      
        scenery.position = Vector3{xPos, 0.0, zPos};
        zPos = length + frequency;
        listOfScenery.push_back(scenery);
    }
    return listOfScenery;
}

void DrawScenery(std::vector<Scenery>& scenery) {
    for (size_t i = 0; i < scenery.size(); i++)
    {
        DrawModel(scenery[i].model, scenery[i].position, scenery[i].scale, WHITE);
    }
    
}

void UpdateScenery(std::vector<Scenery>& listOfScenery, float speed) {
    for (size_t i = 0; i < listOfScenery.size(); i++)
    {
        float distance = Vector3Distance(listOfScenery[i].position, Vector3{0.0, 0.0, 0.0});
        listOfScenery[i].position.z += speed * GetFrameTime();
    
        float scaleFactor = 1.0f / (1.0f + distance * 0.1f);
    
        listOfScenery[i].scale = scaleFactor;
    }
    

    for (auto& object : listOfScenery) {
        if (object.position.z >= 20.0f) {
            float lastObjectZ = listOfScenery[0].position.z;
            for (const auto& o : listOfScenery) {
                if (o.position.z < lastObjectZ) {
                    lastObjectZ = o.position.z;
                }
            }
            object.position.z = lastObjectZ ;
        }
    }
}