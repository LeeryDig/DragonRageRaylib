#ifndef PHYSICS_RIGID_BODY_HPP
#define PHYSICS_RIGID_BODY_HPP

#include "physicsBody.hpp"

namespace physics {

struct RigidBodyDesc : PhysicsBodyDesc {
    float mass;
    float gravityScale;
    float linearDamp;
    float angularDamp;

    RigidBodyDesc()
        : PhysicsBodyDesc(),
          mass(1.0f),
          gravityScale(1.0f),
          linearDamp(0.0f),
          angularDamp(0.0f) {}
};

class RigidBody : public PhysicsBody {
  public:
    RigidBody(std::size_t idIn, const RigidBodyDesc& desc);

    bool IsDynamic() const override;
    void StepMotion(float deltaTime, const Vector3& gravity) override;

    float GetMass() const;
    float GetInverseMass() const;
    float GetGravityScale() const;
    float GetLinearDamp() const;
    float GetAngularDamp() const;

    void ApplyForce(const Vector3& force);
    void ApplyForceAtPoint(const Vector3& force, const Vector3& worldPoint);
    void ApplyTorque(const Vector3& torque);
    void ApplyImpulse(const Vector3& impulse);
    void ClearAccumulators();

  private:
    float mass;
    float inverseMass;
    float gravityScale;
    float linearDamp;
    float angularDamp;
    Vector3 accumulatedForce;
    Vector3 accumulatedTorque;
};

}  // namespace physics

#endif
