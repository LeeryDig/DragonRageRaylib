#ifndef PHYSICS_PHYSICS_BODY_HPP
#define PHYSICS_PHYSICS_BODY_HPP

#include <cstddef>
#include <vector>

#include "collider.hpp"
#include "contact.hpp"
#include "transform3d.hpp"
#include "raylib.h"

namespace physics {

enum class PhysicsBodyType {
    Rigid,
    Static
};

struct PhysicsBodyDesc {
    Transform3D transform;
    CollisionMask defaultLayer;
    CollisionMask defaultMask;
    void* userData;

    PhysicsBodyDesc()
        : transform(Transform3D::Identity()),
          defaultLayer(ToMask(CollisionLayer::World)),
          defaultMask(ToMask(CollisionLayer::All)),
          userData(nullptr) {}
};

class PhysicsBody {
  public:
    PhysicsBody(std::size_t idIn, PhysicsBodyType typeIn, const PhysicsBodyDesc& desc);
    virtual ~PhysicsBody();

    std::size_t GetId() const;
    PhysicsBodyType GetType() const;

    const Transform3D& GetTransform() const;
    void SetTransform(const Transform3D& transformIn);

    Vector3 GetPosition() const;
    void SetPosition(const Vector3& positionIn);

    Vector3 GetLinearVelocity() const;
    void SetLinearVelocity(const Vector3& velocityIn);

    Vector3 GetAngularVelocity() const;
    void SetAngularVelocity(const Vector3& velocityIn);

    CollisionMask GetDefaultLayer() const;
    CollisionMask GetDefaultMask() const;

    void* GetUserData() const;

    Collider* AddCollider(const ColliderDesc& desc);
    const std::vector<Collider>& GetColliders() const;

    const std::vector<BodyContact>& GetContacts() const;
    void ClearContacts();
    void AddContact(const BodyContact& contact);
    bool IsGrounded(float normalThreshold = 0.5f) const;

    virtual bool IsDynamic() const = 0;
    virtual void StepMotion(float deltaTime, const Vector3& gravity) = 0;

  protected:
    Transform3D transform;
    Vector3 linearVelocity;
    Vector3 angularVelocity;

  private:
    std::size_t id;
    PhysicsBodyType type;
    CollisionMask defaultLayer;
    CollisionMask defaultMask;
    void* userData;
    std::vector<Collider> colliders;
    std::vector<BodyContact> contacts;
    std::size_t nextColliderId;
};

}  // namespace physics

#endif
