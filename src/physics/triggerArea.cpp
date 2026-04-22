#include "triggerArea.hpp"

namespace physics {

TriggerArea::TriggerArea(std::size_t idIn, const TriggerAreaDesc& desc)
    : id(idIn),
      transform(desc.transform),
      defaultLayer(desc.defaultLayer),
      defaultMask(desc.defaultMask),
      userData(desc.userData),
      colliders(),
      nextColliderId(1) {}

std::size_t TriggerArea::GetId() const {
    return id;
}

const Transform3D& TriggerArea::GetTransform() const {
    return transform;
}

void TriggerArea::SetTransform(const Transform3D& transformIn) {
    transform = transformIn;
}

void* TriggerArea::GetUserData() const {
    return userData;
}

CollisionMask TriggerArea::GetDefaultLayer() const {
    return defaultLayer;
}

CollisionMask TriggerArea::GetDefaultMask() const {
    return defaultMask;
}

Collider* TriggerArea::AddCollider(const ColliderDesc& desc) {
    ColliderDesc resolvedDesc = desc;
    resolvedDesc.isTrigger = true;
    if (resolvedDesc.layer == 0u) {
        resolvedDesc.layer = defaultLayer;
    }
    if (resolvedDesc.mask == 0u) {
        resolvedDesc.mask = defaultMask;
    }

    colliders.push_back(Collider(nextColliderId++, resolvedDesc));
    return &colliders.back();
}

const std::vector<Collider>& TriggerArea::GetColliders() const {
    return colliders;
}

}  // namespace physics
