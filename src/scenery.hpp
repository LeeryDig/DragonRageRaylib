#ifndef SCENERY_HPP
#define SCENERY_HPP

#include "raylib.h"
#include <raymath.h>
#include <vector>

struct Scenery {
    Model model;
    Vector3 position;
    float scale;
};

std::vector<Scenery> GenerateScenery(float xPos, int total, float frequency);
void DrawScenery(std::vector<Scenery>& listOfScenery);
void UpdateScenery(std::vector<Scenery>& listOfScenery, float speed);


#endif