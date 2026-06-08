#include "interactionRay.hpp"

#include <algorithm>
#include <cmath>

#include "raymath.h"

namespace {
float DistancePointToRay(Vector3 point, Ray ray) {
    Vector3 toPoint = Vector3Subtract(point, ray.position);
    float t = std::max(0.0f, Vector3DotProduct(toPoint, ray.direction));
    Vector3 closest = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    return Vector3Distance(point, closest);
}

float DistanceRayToCapsuleApprox(Ray ray, const CharacterCapsule& capsule) {
    float best = std::min(DistancePointToRay(capsule.bottom, ray), DistancePointToRay(capsule.top, ray));
    for (int i = 1; i < 6; ++i) {
        float t = static_cast<float>(i) / 6.0f;
        Vector3 point = Vector3Lerp(capsule.bottom, capsule.top, t);
        best = std::min(best, DistancePointToRay(point, ray));
    }
    return best;
}
}

Ray BuildInteractionRay(const Camera& camera) {
    Vector3 direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    if (Vector3LengthSqr(direction) <= 0.0001f) direction = Vector3{0.0f, 0.0f, -1.0f};
    return Ray{camera.position, direction};
}

void DrawInteractionRayDebug(const Camera& camera, float length) {
    Ray ray = BuildInteractionRay(camera);
    DrawLine3D(ray.position, Vector3Add(ray.position, Vector3Scale(ray.direction, length)), SKYBLUE);
    DrawSphere(Vector3Add(ray.position, Vector3Scale(ray.direction, length)), 0.035f, SKYBLUE);
}

bool RayHitsInteractionBox(Ray ray, BoundingBox box, float maxDistance, float& hitDistance) {
    RayCollision hit = GetRayCollisionBox(ray, box);
    if (!hit.hit || hit.distance > maxDistance) return false;
    hitDistance = hit.distance;
    return true;
}

bool RayHitsInteractionCapsule(Ray ray, const CharacterCapsule& capsule, float maxDistance, float aimTolerance, float& hitDistance) {
    Vector3 center = Vector3Scale(Vector3Add(capsule.bottom, capsule.top), 0.5f);
    float aimDistance = DistanceRayToCapsuleApprox(ray, capsule) - capsule.radius;
    float cameraDistance = Vector3Distance(ray.position, center);
    if (aimDistance > aimTolerance || cameraDistance > maxDistance) return false;
    hitDistance = cameraDistance;
    return true;
}
