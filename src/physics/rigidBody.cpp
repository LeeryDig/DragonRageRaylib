#include "rigidBody.hpp"

#include "raymath.h"

namespace physics {

namespace {

float ClampMass(float value) {
    return value <= 0.0001f ? 0.0001f : value;
}

Vector3 DampVelocity(const Vector3& velocity, float damp, float deltaTime) {
    float factor = 1.0f / (1.0f + damp * deltaTime);
    return Vector3Scale(velocity, factor);
}

}  // namespace

RigidBody::RigidBody(std::size_t idIn, const RigidBodyDesc& desc)
    : PhysicsBody(idIn, PhysicsBodyType::Rigid, desc),
      mass(ClampMass(desc.mass)),
      inverseMass(1.0f / ClampMass(desc.mass)),
      gravityScale(desc.gravityScale),
      linearDamp(desc.linearDamp),
      angularDamp(desc.angularDamp),
      accumulatedForce{0.0f, 0.0f, 0.0f},
      accumulatedTorque{0.0f, 0.0f, 0.0f} {}

bool RigidBody::IsDynamic() const {
    return true;
}

void RigidBody::StepMotion(float deltaTime, const Vector3& gravity) {
    Vector3 acceleration = Vector3Scale(accumulatedForce, inverseMass);
    acceleration = Vector3Add(acceleration, Vector3Scale(gravity, gravityScale));

    linearVelocity = Vector3Add(linearVelocity, Vector3Scale(acceleration, deltaTime));
    linearVelocity = DampVelocity(linearVelocity, linearDamp, deltaTime);
    angularVelocity = Vector3Add(angularVelocity, Vector3Scale(accumulatedTorque, inverseMass * deltaTime));
    angularVelocity = DampVelocity(angularVelocity, angularDamp, deltaTime);

    transform.position = Vector3Add(transform.position, Vector3Scale(linearVelocity, deltaTime));

    ClearAccumulators();
}

float RigidBody::GetMass() const {
    return mass;
}

float RigidBody::GetInverseMass() const {
    return inverseMass;
}

float RigidBody::GetGravityScale() const {
    return gravityScale;
}

float RigidBody::GetLinearDamp() const {
    return linearDamp;
}

float RigidBody::GetAngularDamp() const {
    return angularDamp;
}

void RigidBody::ApplyForce(const Vector3& force) {
    accumulatedForce = Vector3Add(accumulatedForce, force);
}

void RigidBody::ApplyForceAtPoint(const Vector3& force, const Vector3& worldPoint) {
    accumulatedForce = Vector3Add(accumulatedForce, force);
    Vector3 offset = Vector3Subtract(worldPoint, transform.position);
    accumulatedTorque = Vector3Add(accumulatedTorque, Vector3CrossProduct(offset, force));
}

void RigidBody::ApplyTorque(const Vector3& torque) {
    accumulatedTorque = Vector3Add(accumulatedTorque, torque);
}

void RigidBody::ApplyImpulse(const Vector3& impulse) {
    linearVelocity = Vector3Add(linearVelocity, Vector3Scale(impulse, inverseMass));
}

void RigidBody::ClearAccumulators() {
    accumulatedForce = Vector3{0.0f, 0.0f, 0.0f};
    accumulatedTorque = Vector3{0.0f, 0.0f, 0.0f};
}

}  // namespace physics
