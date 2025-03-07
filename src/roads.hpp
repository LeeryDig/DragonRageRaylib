#ifndef ROADS_HPP
#define ROADS_HPP

#include "raylib.h"
#include <vector>

struct Roads {
    Model model;
    Vector3 position;
    float length;
};

std::vector<Roads> GenerateRoads(int total);

void DrawRoads(std::vector<Roads>& roads);

void UpdateRoads(std::vector<Roads>& roads, float speed);

#endif