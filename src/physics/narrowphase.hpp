#ifndef PHYSICS_NARROWPHASE_HPP
#define PHYSICS_NARROWPHASE_HPP

#include "collider.hpp"
#include "contact.hpp"
#include "physicsBody.hpp"
#include "triggerArea.hpp"

namespace physics {

struct OverlapResult {
    ContactPoint point;
};

bool ComputeBodyBoxOverlap(
    const PhysicsBody& bodyA,
    const Collider& colliderA,
    const PhysicsBody& bodyB,
    const Collider& colliderB,
    OverlapResult& result);

bool ComputeTriggerBodyBoxOverlap(
    const TriggerArea& trigger,
    const Collider& triggerCollider,
    const PhysicsBody& body,
    const Collider& bodyCollider,
    OverlapResult& result);

}  // namespace physics

#endif
