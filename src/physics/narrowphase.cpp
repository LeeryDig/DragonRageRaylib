#include "narrowphase.hpp"

#include <cmath>

#include "raymath.h"

#include "shapes/boxShape.hpp"

namespace physics {

namespace {

struct WorldBox {
    Vector3 center;
    Vector3 halfExtents;
};

bool IsBoxShape(const Collider& collider) {
    return collider.GetShape() && collider.GetShape()->GetType() == ShapeType::Box;
}

WorldBox BuildWorldBox(const Transform3D& ownerTransform, const Collider& collider) {
    const BoxShape* boxShape = static_cast<const BoxShape*>(collider.GetShape().get());

    WorldBox box = {};
    box.center = Vector3Add(ownerTransform.position, collider.GetLocalTransform().position);
    box.halfExtents = Vector3Scale(boxShape->size, 0.5f);
    return box;
}

bool ComputeBoxBoxOverlapInternal(
    const WorldBox& boxA,
    const Vector3& velocityA,
    const WorldBox& boxB,
    const Vector3& velocityB,
    OverlapResult& result) {
    Vector3 delta = Vector3Subtract(boxB.center, boxA.center);
    float overlapX = boxA.halfExtents.x + boxB.halfExtents.x - fabsf(delta.x);
    float overlapY = boxA.halfExtents.y + boxB.halfExtents.y - fabsf(delta.y);
    float overlapZ = boxA.halfExtents.z + boxB.halfExtents.z - fabsf(delta.z);

    if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f) {
        return false;
    }

    result.point.penetrationDepth = overlapX;
    result.point.normal = Vector3{delta.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f};
    result.point.position = Vector3{
        boxA.center.x + result.point.normal.x * boxA.halfExtents.x,
        boxA.center.y,
        boxA.center.z
    };

    if (overlapY < result.point.penetrationDepth) {
        result.point.penetrationDepth = overlapY;
        result.point.normal = Vector3{0.0f, delta.y >= 0.0f ? 1.0f : -1.0f, 0.0f};
        result.point.position = Vector3{
            boxA.center.x,
            boxA.center.y + result.point.normal.y * boxA.halfExtents.y,
            boxA.center.z
        };
    }

    if (overlapZ < result.point.penetrationDepth) {
        result.point.penetrationDepth = overlapZ;
        result.point.normal = Vector3{0.0f, 0.0f, delta.z >= 0.0f ? 1.0f : -1.0f};
        result.point.position = Vector3{
            boxA.center.x,
            boxA.center.y,
            boxA.center.z + result.point.normal.z * boxA.halfExtents.z
        };
    }

    result.point.relativeVelocity = Vector3Subtract(velocityA, velocityB);
    return true;
}

}  // namespace

bool ComputeBodyBoxOverlap(
    const PhysicsBody& bodyA,
    const Collider& colliderA,
    const PhysicsBody& bodyB,
    const Collider& colliderB,
    OverlapResult& result) {
    if (!IsBoxShape(colliderA) || !IsBoxShape(colliderB)) {
        return false;
    }

    WorldBox boxA = BuildWorldBox(bodyA.GetTransform(), colliderA);
    WorldBox boxB = BuildWorldBox(bodyB.GetTransform(), colliderB);
    return ComputeBoxBoxOverlapInternal(
        boxA,
        bodyA.GetLinearVelocity(),
        boxB,
        bodyB.GetLinearVelocity(),
        result);
}

bool ComputeTriggerBodyBoxOverlap(
    const TriggerArea& trigger,
    const Collider& triggerCollider,
    const PhysicsBody& body,
    const Collider& bodyCollider,
    OverlapResult& result) {
    if (!IsBoxShape(triggerCollider) || !IsBoxShape(bodyCollider)) {
        return false;
    }

    WorldBox triggerBox = BuildWorldBox(trigger.GetTransform(), triggerCollider);
    WorldBox bodyBox = BuildWorldBox(body.GetTransform(), bodyCollider);
    return ComputeBoxBoxOverlapInternal(
        triggerBox,
        Vector3{0.0f, 0.0f, 0.0f},
        bodyBox,
        body.GetLinearVelocity(),
        result);
}

}  // namespace physics
