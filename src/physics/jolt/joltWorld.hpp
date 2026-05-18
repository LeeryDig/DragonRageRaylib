#ifndef PHYSICS_JOLT_JOLT_WORLD_HPP
#define PHYSICS_JOLT_JOLT_WORLD_HPP

#include <memory>
#include <vector>

#include "raylib.h"
#include "level/levelData.hpp"
#include "personController.hpp"

namespace JPH {
class Body;
class CharacterVirtual;
class CharacterVirtualSettings;
class JobSystemThreadPool;
class PhysicsSystem;
class Shape;
class TempAllocatorImpl;
}

namespace physics_jolt {

struct RaycastHit {
    bool hit;
    Vector3 position;
    Vector3 normal;
    float distance;

    RaycastHit()
        : hit(false), position{0.0f, 0.0f, 0.0f}, normal{0.0f, 1.0f, 0.0f}, distance(0.0f) {}
};

class JoltWorld {
  public:
    JoltWorld();
    ~JoltWorld();

    bool Init();
    void Shutdown();
    void Clear();

    void LoadLevel(const LevelData& level);
    void CreateCharacter(const PersonConfig& config, Vector3 position);
    void SetCharacterPosition(Vector3 position);
    void UpdateCharacter(PersonState& person, const PersonConfig& config, Vector3 desiredHorizontalVelocity, float deltaTime);

    bool Raycast(Vector3 origin, Vector3 direction, float distance, RaycastHit& hit) const;
    bool IsReady() const;
    bool HasCharacter() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace physics_jolt

#endif
