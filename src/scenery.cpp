#include "scenery.hpp"

#include <iostream>

std::vector<Scenery> GenerateScenery(float xPos, int total, float frequency) {
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
        zPos += length + frequency;
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
        if (listOfScenery[i].position.z >= 20) {
            listOfScenery[i].position.z = -30.0;
        }

        listOfScenery[i].position.z += (speed * 5) * GetFrameTime();

        float distance = Vector3Distance(listOfScenery[i].position, Vector3{0.0, 0.0, 0.0});
        float maxDistance = 40.0f; 
    
        float scaleFactor = 1.0f - (distance / maxDistance);
        scaleFactor = fmaxf(0.0f, fminf(scaleFactor, 1.0f));
    
        listOfScenery[i].scale = scaleFactor;
    }
}