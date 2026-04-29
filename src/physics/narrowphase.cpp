#include "narrowphase.hpp"

#include <cmath>

#include "raymath.h"

#include "shapes/boxShape.hpp"

namespace physics {

namespace {

const float kContactEpsilon = 0.02f;
const float kParallelAxisThreshold = 0.0001f;

struct OrientedWorldBox {
    Vector3 center;
    Vector3 halfExtents;
    Vector3 axes[3];
};

bool IsBoxShape(const Collider& collider) {
    return collider.GetShape() && collider.GetShape()->GetType() == ShapeType::Box;
}

OrientedWorldBox BuildWorldBox(const Transform3D& ownerTransform, const Collider& collider) {
    const BoxShape* boxShape = static_cast<const BoxShape*>(collider.GetShape().get());

    OrientedWorldBox box = {};
    box.center = Vector3Add(
        ownerTransform.position,
        Vector3RotateByQuaternion(collider.GetLocalTransform().position, ownerTransform.rotation));
    box.halfExtents = Vector3Scale(boxShape->size, 0.5f);
    box.axes[0] = Vector3Normalize(Vector3RotateByQuaternion(Vector3{1.0f, 0.0f, 0.0f}, ownerTransform.rotation));
    box.axes[1] = Vector3Normalize(Vector3RotateByQuaternion(Vector3{0.0f, 1.0f, 0.0f}, ownerTransform.rotation));
    box.axes[2] = Vector3Normalize(Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, 1.0f}, ownerTransform.rotation));
    return box;
}

float ProjectBoxOntoAxis(const OrientedWorldBox& box, const Vector3& axis) {
    return
        box.halfExtents.x * fabsf(Vector3DotProduct(box.axes[0], axis)) +
        box.halfExtents.y * fabsf(Vector3DotProduct(box.axes[1], axis)) +
        box.halfExtents.z * fabsf(Vector3DotProduct(box.axes[2], axis));
}

bool TestSeparationAxis(
    const OrientedWorldBox& boxA,
    const OrientedWorldBox& boxB,
    const Vector3& centerDelta,
    const Vector3& axis,
    float& minOverlap,
    Vector3& bestAxis) {
    float axisLengthSqr = Vector3LengthSqr(axis);
    if (axisLengthSqr <= kParallelAxisThreshold) {
        return true;
    }

    Vector3 normalizedAxis = Vector3Scale(axis, 1.0f / sqrtf(axisLengthSqr));
    float distance = fabsf(Vector3DotProduct(centerDelta, normalizedAxis));
    float radiusA = ProjectBoxOntoAxis(boxA, normalizedAxis);
    float radiusB = ProjectBoxOntoAxis(boxB, normalizedAxis);
    float overlap = radiusA + radiusB - distance;
    if (overlap < -kContactEpsilon) {
        return false;
    }

    overlap = overlap < 0.0f ? 0.0f : overlap;
    if (overlap < minOverlap) {
        minOverlap = overlap;
        bestAxis = normalizedAxis;
        if (Vector3DotProduct(centerDelta, bestAxis) < 0.0f) {
            bestAxis = Vector3Negate(bestAxis);
        }
    }

    return true;
}

Vector3 GetSupportPoint(const OrientedWorldBox& box, const Vector3& direction) {
    Vector3 point = box.center;
    float halfExtents[3] = {box.halfExtents.x, box.halfExtents.y, box.halfExtents.z};
    for (int i = 0; i < 3; ++i) {
        float sign = Vector3DotProduct(box.axes[i], direction) >= 0.0f ? 1.0f : -1.0f;
        point = Vector3Add(point, Vector3Scale(box.axes[i], halfExtents[i] * sign));
    }
    return point;
}

bool ComputeBoxBoxOverlapInternal(
    const OrientedWorldBox& boxA,
    const Vector3& velocityA,
    const OrientedWorldBox& boxB,
    const Vector3& velocityB,
    OverlapResult& result) {
    Vector3 centerDelta = Vector3Subtract(boxB.center, boxA.center);
    float minOverlap = INFINITY;
    Vector3 bestAxis = Vector3{0.0f, 1.0f, 0.0f};

    for (int i = 0; i < 3; ++i) {
        if (!TestSeparationAxis(boxA, boxB, centerDelta, boxA.axes[i], minOverlap, bestAxis)) {
            return false;
        }
    }

    for (int i = 0; i < 3; ++i) {
        if (!TestSeparationAxis(boxA, boxB, centerDelta, boxB.axes[i], minOverlap, bestAxis)) {
            return false;
        }
    }

    for (int axisA = 0; axisA < 3; ++axisA) {
        for (int axisB = 0; axisB < 3; ++axisB) {
            Vector3 crossAxis = Vector3CrossProduct(boxA.axes[axisA], boxB.axes[axisB]);
            if (!TestSeparationAxis(boxA, boxB, centerDelta, crossAxis, minOverlap, bestAxis)) {
                return false;
            }
        }
    }

    Vector3 supportA = GetSupportPoint(boxA, bestAxis);
    Vector3 supportB = GetSupportPoint(boxB, Vector3Negate(bestAxis));
    result.point.penetrationDepth = minOverlap;
    result.point.normal = bestAxis;
    result.point.position = Vector3Scale(Vector3Add(supportA, supportB), 0.5f);
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

    OrientedWorldBox boxA = BuildWorldBox(bodyA.GetTransform(), colliderA);
    OrientedWorldBox boxB = BuildWorldBox(bodyB.GetTransform(), colliderB);
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

    OrientedWorldBox triggerBox = BuildWorldBox(trigger.GetTransform(), triggerCollider);
    OrientedWorldBox bodyBox = BuildWorldBox(body.GetTransform(), bodyCollider);
    return ComputeBoxBoxOverlapInternal(
        triggerBox,
        Vector3{0.0f, 0.0f, 0.0f},
        bodyBox,
        body.GetLinearVelocity(),
        result);
}

}  // namespace physics
