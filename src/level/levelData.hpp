#ifndef LEVEL_LEVEL_DATA_HPP
#define LEVEL_LEVEL_DATA_HPP

#include <string>
#include <vector>

#include "raylib.h"

struct LevelBoxVolume {
    std::string name;
    Vector3 position;
    Quaternion rotation;
    Vector3 size;
};

struct LevelRoadSurface {
    std::string name;
    Vector3 position;
    Quaternion rotation;
    Vector3 size;
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
};

struct LevelCheckpoint : LevelBoxVolume {
    int index;
};

struct LevelSpawn {
    Vector3 position;
    Quaternion rotation;
    bool valid;

    LevelSpawn()
        : position{0.0f, 1.0f, 0.0f},
          rotation{0.0f, 0.0f, 0.0f, 1.0f},
          valid(false) {}
};

struct LevelData {
    std::string name;
    std::string path;
    Model visualModel;
    bool visualLoaded;
    std::vector<LevelBoxVolume> colliders;
    std::vector<LevelRoadSurface> roadSurfaces;
    std::vector<LevelCheckpoint> checkpoints;
    LevelSpawn playerSpawn;
    LevelBoxVolume finishLine;
    bool hasFinishLine;

    LevelData()
        : visualModel(),
          visualLoaded(false),
          finishLine(),
          hasFinishLine(false) {}
};

#endif
