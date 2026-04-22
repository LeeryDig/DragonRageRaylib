#ifndef PHYSICS_TRIGGER_AREA_HPP
#define PHYSICS_TRIGGER_AREA_HPP

#include <cstddef>
#include <vector>

#include "collider.hpp"
#include "transform3d.hpp"

namespace physics {

struct TriggerAreaDesc {
    Transform3D transform;
    CollisionMask defaultLayer;
    CollisionMask defaultMask;
    void* userData;

    TriggerAreaDesc()
        : transform(Transform3D::Identity()),
          defaultLayer(ToMask(CollisionLayer::Trigger)),
          defaultMask(ToMask(CollisionLayer::Vehicle)),
          userData(nullptr) {}
};

class TriggerArea {
  public:
    TriggerArea(std::size_t idIn, const TriggerAreaDesc& desc);

    std::size_t GetId() const;
    const Transform3D& GetTransform() const;
    void SetTransform(const Transform3D& transformIn);

    void* GetUserData() const;
    CollisionMask GetDefaultLayer() const;
    CollisionMask GetDefaultMask() const;

    Collider* AddCollider(const ColliderDesc& desc);
    const std::vector<Collider>& GetColliders() const;

  private:
    std::size_t id;
    Transform3D transform;
    CollisionMask defaultLayer;
    CollisionMask defaultMask;
    void* userData;
    std::vector<Collider> colliders;
    std::size_t nextColliderId;
};

}  // namespace physics

#endif
