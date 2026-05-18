#include "narrowphase.hpp"

#include <cmath>

#include "raymath.h"

#include "shapes/boxShape.hpp"
#include "shapes/meshShape.hpp"

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

bool IsMeshShape(const Collider& collider) {
    return collider.GetShape() && collider.GetShape()->GetType() == ShapeType::Mesh;
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

bool PointInTriangle(const Vector3& point, const Vector3& a, const Vector3& b, const Vector3& c) {
    Vector3 v0 = Vector3Subtract(c, a);
    Vector3 v1 = Vector3Subtract(b, a);
    Vector3 v2 = Vector3Subtract(point, a);
    float dot00 = Vector3DotProduct(v0, v0);
    float dot01 = Vector3DotProduct(v0, v1);
    float dot02 = Vector3DotProduct(v0, v2);
    float dot11 = Vector3DotProduct(v1, v1);
    float dot12 = Vector3DotProduct(v1, v2);
    float denom = dot00 * dot11 - dot01 * dot01;
    if (fabsf(denom) <= 0.000001f) return false;
    float invDenom = 1.0f / denom;
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    return u >= -0.001f && v >= -0.001f && (u + v) <= 1.001f;
}

std::vector<Vector3> GetWorldBoxCorners(const OrientedWorldBox& box) {
    std::vector<Vector3> corners;
    corners.reserve(8);
    float halfExtents[3] = {box.halfExtents.x, box.halfExtents.y, box.halfExtents.z};
    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            for (int z = -1; z <= 1; z += 2) {
                Vector3 corner = box.center;
                corner = Vector3Add(corner, Vector3Scale(box.axes[0], halfExtents[0] * static_cast<float>(x)));
                corner = Vector3Add(corner, Vector3Scale(box.axes[1], halfExtents[1] * static_cast<float>(y)));
                corner = Vector3Add(corner, Vector3Scale(box.axes[2], halfExtents[2] * static_cast<float>(z)));
                corners.push_back(corner);
            }
        }
    }
    return corners;
}

bool ComputeBoxMeshOverlapInternal(
    const OrientedWorldBox& box,
    const Vector3& boxVelocity,
    const MeshShape& meshShape,
    bool boxIsBodyA,
    OverlapResult& result) {
    const std::vector<Vector3>& vertices = meshShape.GetVertices();
    const std::vector<unsigned int>& indices = meshShape.GetIndices();
    if (vertices.empty() || indices.size() < 3) return false;

    std::vector<Vector3> corners = GetWorldBoxCorners(box);
    bool found = false;
    float bestPenetration = 0.0f;
    ContactPoint bestPoint = {};

    for (std::size_t t = 0; t + 2 < indices.size(); t += 3) {
        unsigned int ia = indices[t];
        unsigned int ib = indices[t + 1];
        unsigned int ic = indices[t + 2];
        if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size()) continue;

        Vector3 a = vertices[ia];
        Vector3 b = vertices[ib];
        Vector3 c = vertices[ic];
        Vector3 normal = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
        float normalLength = Vector3Length(normal);
        if (normalLength <= 0.000001f) continue;
        normal = Vector3Scale(normal, 1.0f / normalLength);

        Vector3 triCenter = Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
        if (Vector3DotProduct(Vector3Subtract(box.center, triCenter), normal) < 0.0f) {
            normal = Vector3Negate(normal);
        }

        for (std::size_t i = 0; i < corners.size(); ++i) {
            float distance = Vector3DotProduct(Vector3Subtract(corners[i], a), normal);
            if (distance > kContactEpsilon) continue;
            Vector3 projected = Vector3Subtract(corners[i], Vector3Scale(normal, distance));
            if (!PointInTriangle(projected, a, b, c)) continue;

            float penetration = -distance;
            if (!found || penetration > bestPenetration) {
                found = true;
                bestPenetration = penetration;
                bestPoint.penetrationDepth = penetration;
                bestPoint.position = projected;
                bestPoint.normal = boxIsBodyA ? Vector3Negate(normal) : normal;
                bestPoint.relativeVelocity = boxIsBodyA ? boxVelocity : Vector3Negate(boxVelocity);
            }
        }
    }

    if (!found) return false;
    result.point = bestPoint;
    return true;
}

}  // namespace

bool ComputeBodyOverlap(
    const PhysicsBody& bodyA,
    const Collider& colliderA,
    const PhysicsBody& bodyB,
    const Collider& colliderB,
    OverlapResult& result) {
    if (IsBoxShape(colliderA) && IsBoxShape(colliderB)) {
        OrientedWorldBox boxA = BuildWorldBox(bodyA.GetTransform(), colliderA);
        OrientedWorldBox boxB = BuildWorldBox(bodyB.GetTransform(), colliderB);
        return ComputeBoxBoxOverlapInternal(
            boxA,
            bodyA.GetLinearVelocity(),
            boxB,
            bodyB.GetLinearVelocity(),
            result);
    }

    if (IsBoxShape(colliderA) && IsMeshShape(colliderB)) {
        OrientedWorldBox boxA = BuildWorldBox(bodyA.GetTransform(), colliderA);
        const MeshShape* meshShape = static_cast<const MeshShape*>(colliderB.GetShape().get());
        return ComputeBoxMeshOverlapInternal(boxA, bodyA.GetLinearVelocity(), *meshShape, true, result);
    }

    if (IsMeshShape(colliderA) && IsBoxShape(colliderB)) {
        OrientedWorldBox boxB = BuildWorldBox(bodyB.GetTransform(), colliderB);
        const MeshShape* meshShape = static_cast<const MeshShape*>(colliderA.GetShape().get());
        return ComputeBoxMeshOverlapInternal(boxB, bodyB.GetLinearVelocity(), *meshShape, false, result);
    }

    return false;
}

bool ComputeBodyBoxOverlap(
    const PhysicsBody& bodyA,
    const Collider& colliderA,
    const PhysicsBody& bodyB,
    const Collider& colliderB,
    OverlapResult& result) {
    return ComputeBodyOverlap(bodyA, colliderA, bodyB, colliderB, result);
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
