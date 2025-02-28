#ifndef SCENERY_HPP
#define SCENERY_HPP

#include "raylib.h"
#include <vector>

struct Scenery {
    Model model;
    Vector3 position;
    float beginDistance;
    float endDistance;
    float frenquecy;
};

std::vector<Scenery> GenerateScenery(int num, float beginDistance, float endDistance, float frequency);
void DrawScenery(std::vector<Scenery>& listOfScenery, float distance);
void UpdateScenery(std::vector<Scenery>& listOfScenery, float speed);


#endif