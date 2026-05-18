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

struct LevelCollisionMesh {
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

struct LevelSkybox {
    std::string path;
    Model model;
    TextureCubemap texture;
    Shader shader;
    bool loaded;

    LevelSkybox()
        : model(),
          texture(),
          shader(),
          loaded(false) {}
};

struct LevelData {
    std::string name;
    std::string path;
    Model visualModel;
    bool visualLoaded;
    LevelSkybox skybox;
    Vector3 rootPosition;
    Quaternion rootRotation;
    std::vector<LevelRenderPart> renderParts;
    std::vector<LevelDebugNode> debugNodes;
    std::vector<LevelDebugNode> originalDebugNodes;
    std::vector<LevelDebugGroup> debugGroups;
    std::vector<LevelBoxVolume> originalColliders;
    std::vector<LevelCollisionMesh> originalCollisionMeshes;
    std::vector<LevelCheckpoint> originalCheckpoints;
    std::vector<LevelBoxVolume> originalTriggers;
    LevelSpawn originalPlayerSpawn;
    LevelBoxVolume originalFinishLine;
    std::vector<LevelBoxVolume> colliders;
    std::vector<LevelCollisionMesh> collisionMeshes;
    std::vector<LevelCheckpoint> checkpoints;
    std::vector<LevelBoxVolume> triggers;
    LevelSpawn playerSpawn;
    LevelBoxVolume finishLine;
    bool hasFinishLine;

    LevelData()
        : visualModel(),
          visualLoaded(false),
          skybox(),
          rootPosition{0.0f, 0.0f, 0.0f},
          rootRotation{0.0f, 0.0f, 0.0f, 1.0f},
          originalFinishLine(),
          finishLine(),
          hasFinishLine(false) {}
};

#endif
