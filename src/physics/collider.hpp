#ifndef PHYSICS_COLLIDER_HPP
#define PHYSICS_COLLIDER_HPP

#include <cstddef>
#include <memory>

#include "collisionLayers.hpp"
#include "physicsMaterial.hpp"
#include "shape.hpp"
#include "transform3d.hpp"

namespace physics {

struct ColliderDesc {
    Transform3D localTransform;
    std::shared_ptr<const Shape> shape;
    PhysicsMaterial material;
    CollisionMask layer;
    CollisionMask mask;
    bool isTrigger;

    ColliderDesc()
        : localTransform(Transform3D::Identity()),
          shape(),
          material(),
          layer(0u),
          mask(0u),
          isTrigger(false) {}
};

class Collider {
  public:
    Collider(std::size_t idIn, const ColliderDesc& desc);

    std::size_t GetId() const;
    const Transform3D& GetLocalTransform() const;
    const std::shared_ptr<const Shape>& GetShape() const;
    const PhysicsMaterial& GetMaterial() const;
    CollisionMask GetLayer() const;
    CollisionMask GetMask() const;
    bool IsTrigger() const;

  private:
    std::size_t id;
    Transform3D localTransform;
    std::shared_ptr<const Shape> shape;
    PhysicsMaterial material;
    CollisionMask layer;
    CollisionMask mask;
    bool isTrigger;
};

}  // namespace physics

#endif
