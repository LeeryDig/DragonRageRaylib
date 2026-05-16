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

enum class LevelDebugNodeKind {
    Visual,
    Collider,
    Icon,
    Spawn,
    Trigger,
    Invisible,
    Other
};

struct LevelRenderPart {
    int meshIndex;
    Matrix transform;
};

struct LevelDebugNode {
    std::string name;
    LevelDebugNodeKind kind;
    int meshIndex;
    int groupId;
    int runtimeIndex;
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    Vector3 size;
};

struct LevelDebugGroup {
    std::string name;
    Vector3 positionOffset;
    Quaternion rotationOffset;
    Vector3 scaleMultiplier;

    LevelDebugGroup()
        : positionOffset{0.0f, 0.0f, 0.0f},
          rotationOffset{0.0f, 0.0f, 0.0f, 1.0f},
          scaleMultiplier{1.0f, 1.0f, 1.0f} {}
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
    Vector3 rootPosition;
    Quaternion rootRotation;
    std::vector<LevelRenderPart> renderParts;
    std::vector<LevelDebugNode> debugNodes;
    std::vector<LevelDebugNode> originalDebugNodes;
    std::vector<LevelDebugGroup> debugGroups;
    std::vector<LevelBoxVolume> originalColliders;
    std::vector<LevelRoadSurface> originalRoadSurfaces;
    std::vector<LevelCheckpoint> originalCheckpoints;
    LevelSpawn originalPlayerSpawn;
    LevelBoxVolume originalFinishLine;
    std::vector<LevelBoxVolume> colliders;
    std::vector<LevelRoadSurface> roadSurfaces;
    std::vector<LevelCheckpoint> checkpoints;
    LevelSpawn playerSpawn;
    LevelBoxVolume finishLine;
    bool hasFinishLine;

    LevelData()
        : visualModel(),
          visualLoaded(false),
          rootPosition{0.0f, 0.0f, 0.0f},
          rootRotation{0.0f, 0.0f, 0.0f, 1.0f},
          originalFinishLine(),
          finishLine(),
          hasFinishLine(false) {}
};

#endif
