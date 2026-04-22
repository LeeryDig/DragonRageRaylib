#include "physicsBody.hpp"

namespace physics {

PhysicsBody::PhysicsBody(std::size_t idIn, PhysicsBodyType typeIn, const PhysicsBodyDesc& desc)
    : transform(desc.transform),
      linearVelocity{0.0f, 0.0f, 0.0f},
      angularVelocity{0.0f, 0.0f, 0.0f},
      id(idIn),
      type(typeIn),
      defaultLayer(desc.defaultLayer),
      defaultMask(desc.defaultMask),
      userData(desc.userData),
      colliders(),
      contacts(),
      nextColliderId(1) {}

PhysicsBody::~PhysicsBody() {}

std::size_t PhysicsBody::GetId() const {
    return id;
}

PhysicsBodyType PhysicsBody::GetType() const {
    return type;
}

const Transform3D& PhysicsBody::GetTransform() const {
    return transform;
}

void PhysicsBody::SetTransform(const Transform3D& transformIn) {
    transform = transformIn;
}

Vector3 PhysicsBody::GetPosition() const {
    return transform.position;
}

void PhysicsBody::SetPosition(const Vector3& positionIn) {
    transform.position = positionIn;
}

Vector3 PhysicsBody::GetLinearVelocity() const {
    return linearVelocity;
}

void PhysicsBody::SetLinearVelocity(const Vector3& velocityIn) {
    linearVelocity = velocityIn;
}

Vector3 PhysicsBody::GetAngularVelocity() const {
    return angularVelocity;
}

void PhysicsBody::SetAngularVelocity(const Vector3& velocityIn) {
    angularVelocity = velocityIn;
}

CollisionMask PhysicsBody::GetDefaultLayer() const {
    return defaultLayer;
}

CollisionMask PhysicsBody::GetDefaultMask() const {
    return defaultMask;
}

void* PhysicsBody::GetUserData() const {
    return userData;
}

Collider* PhysicsBody::AddCollider(const ColliderDesc& desc) {
    ColliderDesc resolvedDesc = desc;
    if (resolvedDesc.layer == 0u) {
        resolvedDesc.layer = defaultLayer;
    }
    if (resolvedDesc.mask == 0u) {
        resolvedDesc.mask = defaultMask;
    }

    colliders.push_back(Collider(nextColliderId++, resolvedDesc));
    return &colliders.back();
}

const std::vector<Collider>& PhysicsBody::GetColliders() const {
    return colliders;
}

const std::vector<BodyContact>& PhysicsBody::GetContacts() const {
    return contacts;
}

void PhysicsBody::ClearContacts() {
    contacts.clear();
}

void PhysicsBody::AddContact(const BodyContact& contact) {
    contacts.push_back(contact);
}

bool PhysicsBody::IsGrounded(float normalThreshold) const {
    for (std::size_t i = 0; i < contacts.size(); ++i) {
        if (contacts[i].point.normal.y > normalThreshold) {
            return true;
        }
    }
    return false;
}

}  // namespace physics
