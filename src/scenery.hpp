#ifndef SCENERY_HPP
#define SCENERY_HPP

#include "raylib.h"
#include <vector>

struct Scenery {
    Model model;
    float beginDistance;
    float endDistance;
    float frenquecy;
};

void DrawScenery(std::vector<Scenery>& scenery);
void UpdateScenery(std::vector<Scenery>& scenery, float speed);


#endif