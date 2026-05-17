#include "personCollision.hpp"

#include <algorithm>
#include <cmath>

#include "raymath.h"

namespace {

const float WALKABLE_MIN_Y = 0.64f; // ~50 degrees max slope

Vector3 ClosestPointOnTriangle(Vector3 point, Vector3 a, Vector3 b, Vector3 c) {
    Vector3 ab = Vector3Subtract(b, a);
    Vector3 ac = Vector3Subtract(c, a);
    Vector3 ap = Vector3Subtract(point, a);
    float d1 = Vector3DotProduct(ab, ap);
    float d2 = Vector3DotProduct(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    Vector3 bp = Vector3Subtract(point, b);
    float d3 = Vector3DotProduct(ab, bp);
    float d4 = Vector3DotProduct(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return Vector3Add(a, Vector3Scale(ab, v));
    }

    Vector3 cp = Vector3Subtract(point, c);
    float d5 = Vector3DotProduct(ab, cp);
    float d6 = Vector3DotProduct(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return Vector3Add(a, Vector3Scale(ac, w));
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return Vector3Add(b, Vector3Scale(Vector3Subtract(c, b), w));
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return Vector3Add(a, Vector3Add(Vector3Scale(ab, v), Vector3Scale(ac, w)));
}

bool TryVerticalGroundHit(
    const LevelCollisionMesh& mesh,
    const Vector3& point,
    Vector3& groundPoint,
    Vector3& groundNormal) {
    Ray ray = Ray{Vector3Add(point, Vector3{0.0f, 1000.0f, 0.0f}), Vector3{0.0f, -1.0f, 0.0f}};
    bool hit = false;
    float bestDistance = 2000.0f;

    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        unsigned int ia = mesh.indices[i];
        unsigned int ib = mesh.indices[i + 1];
        unsigned int ic = mesh.indices[i + 2];
        if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() || ic >= mesh.vertices.size()) continue;

        RayCollision collision = GetRayCollisionTriangle(ray, mesh.vertices[ia], mesh.vertices[ib], mesh.vertices[ic]);
        if (!collision.hit || collision.distance < 0.0f || collision.distance > bestDistance) continue;

        Vector3 normal = collision.normal;
        if (Vector3LengthSqr(normal) <= 0.0001f) {
            normal = Vector3Normalize(Vector3CrossProduct(
                Vector3Subtract(mesh.vertices[ib], mesh.vertices[ia]),
                Vector3Subtract(mesh.vertices[ic], mesh.vertices[ia])));
        }
        if (normal.y < 0.0f) normal = Vector3Negate(normal);
        if (normal.y < WALKABLE_MIN_Y) continue;

        hit = true;
        bestDistance = collision.distance;
        groundPoint = collision.point;
        groundNormal = normal;
    }

    return hit;
}

} // namespace

void ResolvePersonLevelCollisions(const LevelData& level, PersonState& person, const PersonConfig& config) {
    const float radius = config.capsuleRadius;
    const float height = config.capsuleHeight;
    const float radiusSqr = radius * radius;
    const int iterations = 3;

    for (int iteration = 0; iteration < iterations; ++iteration) {
        bool anyHit = false;
        Vector3 sphereCenters[3] = {
            Vector3Add(person.position, Vector3{0.0f, radius, 0.0f}),
            Vector3Add(person.position, Vector3{0.0f, radius + height * 0.5f, 0.0f}),
            Vector3Add(person.position, Vector3{0.0f, radius + height, 0.0f})
        };

        for (std::size_t meshIndex = 0; meshIndex < level.collisionMeshes.size(); ++meshIndex) {
            const LevelCollisionMesh& mesh = level.collisionMeshes[meshIndex];
            for (std::size_t tri = 0; tri + 2 < mesh.indices.size(); tri += 3) {
                unsigned int ia = mesh.indices[tri];
                unsigned int ib = mesh.indices[tri + 1];
                unsigned int ic = mesh.indices[tri + 2];
                if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() || ic >= mesh.vertices.size()) continue;

                Vector3 a = mesh.vertices[ia];
                Vector3 b = mesh.vertices[ib];
                Vector3 c = mesh.vertices[ic];
                Vector3 triangleNormal = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
                if (Vector3LengthSqr(triangleNormal) <= 0.0001f) continue;
                triangleNormal = Vector3Normalize(triangleNormal);

                for (int s = 0; s < 3; ++s) {
                    Vector3 closest = ClosestPointOnTriangle(sphereCenters[s], a, b, c);
                    Vector3 delta = Vector3Subtract(sphereCenters[s], closest);
                    float distSqr = Vector3LengthSqr(delta);
                    if (distSqr >= radiusSqr) continue;

                    float distance = sqrtf(std::max(distSqr, 0.000001f));
                    Vector3 normal = distance > 0.0001f ? Vector3Scale(delta, 1.0f / distance) : triangleNormal;
                    if (normal.y > 0.0f && normal.y < WALKABLE_MIN_Y) {
                        normal.y = 0.0f;
                        if (Vector3LengthSqr(normal) <= 0.0001f) continue;
                        normal = Vector3Normalize(normal);
                    }

                    float penetration = radius - distance;
                    person.position = Vector3Add(person.position, Vector3Scale(normal, penetration));
                    float velocityIntoSurface = Vector3DotProduct(person.velocity, normal);
                    if (velocityIntoSurface < 0.0f) {
                        person.velocity = Vector3Subtract(person.velocity, Vector3Scale(normal, velocityIntoSurface));
                    }
                    anyHit = true;
                    break;
                }
            }
        }

        if (!anyHit) break;
    }
}

void ApplyPersonGroundSnap(const LevelData& level, PersonState& person, const PersonConfig& config) {
    Vector3 groundSamples[5] = {
        person.position,
        Vector3Add(person.position, Vector3{config.capsuleRadius, 0.0f, 0.0f}),
        Vector3Add(person.position, Vector3{-config.capsuleRadius, 0.0f, 0.0f}),
        Vector3Add(person.position, Vector3{0.0f, 0.0f, config.capsuleRadius}),
        Vector3Add(person.position, Vector3{0.0f, 0.0f, -config.capsuleRadius})
    };

    bool hasGround = false;
    float bestGroundY = -INFINITY;
    for (int i = 0; i < 5; ++i) {
        for (std::size_t meshIndex = 0; meshIndex < level.collisionMeshes.size(); ++meshIndex) {
            Vector3 groundPoint = Vector3Zero();
            Vector3 groundNormal = Vector3{0.0f, 1.0f, 0.0f};
            if (TryVerticalGroundHit(level.collisionMeshes[meshIndex], groundSamples[i], groundPoint, groundNormal) && groundPoint.y > bestGroundY) {
                hasGround = true;
                bestGroundY = groundPoint.y;
            }
        }
    }

    person.grounded = false;
    if (!hasGround) return;

    float penetration = bestGroundY - person.position.y;
    bool fallingOrResting = person.velocity.y <= 0.0f;
    if (penetration >= -config.groundSnapDistance && fallingOrResting) {
        person.position.y += penetration;
        person.velocity.y = 0.0f;
        person.grounded = true;
    }
}
