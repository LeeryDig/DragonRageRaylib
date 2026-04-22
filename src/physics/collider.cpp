#include "collider.hpp"

namespace physics {

Collider::Collider(std::size_t idIn, const ColliderDesc& desc)
    : id(idIn),
      localTransform(desc.localTransform),
      shape(desc.shape),
      material(desc.material),
      layer(desc.layer),
      mask(desc.mask),
      isTrigger(desc.isTrigger) {}

std::size_t Collider::GetId() const {
    return id;
}

const Transform3D& Collider::GetLocalTransform() const {
    return localTransform;
}

const std::shared_ptr<const Shape>& Collider::GetShape() const {
    return shape;
}

const PhysicsMaterial& Collider::GetMaterial() const {
    return material;
}

CollisionMask Collider::GetLayer() const {
    return layer;
}

CollisionMask Collider::GetMask() const {
    return mask;
}

bool Collider::IsTrigger() const {
    return isTrigger;
}

}  // namespace physics
