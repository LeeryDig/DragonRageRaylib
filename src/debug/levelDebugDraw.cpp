#include "debug/levelDebugDraw.hpp"

#include <algorithm>

#include "raymath.h"

void DrawDebugLevelBoxWire(const LevelBoxVolume& box, Color color) {
    Vector3 half = Vector3Scale(box.size, 0.5f);
    Vector3 corners[8];
    int index = 0;
    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            for (int z = -1; z <= 1; z += 2) {
                Vector3 local = Vector3{half.x * static_cast<float>(x), half.y * static_cast<float>(y), half.z * static_cast<float>(z)};
                corners[index++] = Vector3Add(box.position, Vector3RotateByQuaternion(local, box.rotation));
            }
        }
    }

    const int edges[12][2] = {
        {0, 1}, {0, 2}, {0, 4}, {3, 1}, {3, 2}, {3, 7},
        {5, 1}, {5, 4}, {5, 7}, {6, 2}, {6, 4}, {6, 7}
    };
    for (int i = 0; i < 12; ++i) {
        DrawLine3D(corners[edges[i][0]], corners[edges[i][1]], color);
    }
}

void DrawLevelCollidersDebug(const LevelData& level) {
    for (std::size_t i = 0; i < level.colliders.size(); ++i) {
        DrawDebugLevelBoxWire(level.colliders[i], GREEN);
    }

    for (std::size_t i = 0; i < level.checkpoints.size(); ++i) {
        DrawDebugLevelBoxWire(level.checkpoints[i], PURPLE);
    }
    for (std::size_t i = 0; i < level.triggers.size(); ++i) {
        DrawDebugLevelBoxWire(level.triggers[i], PURPLE);
    }
    if (level.hasFinishLine) {
        DrawDebugLevelBoxWire(level.finishLine, PURPLE);
    }

    for (std::size_t i = 0; i < level.collisionMeshes.size(); ++i) {
        const LevelCollisionMesh& mesh = level.collisionMeshes[i];
        for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
            unsigned int ia = mesh.indices[t];
            unsigned int ib = mesh.indices[t + 1];
            unsigned int ic = mesh.indices[t + 2];
            if (ia < mesh.vertices.size() && ib < mesh.vertices.size() && ic < mesh.vertices.size()) {
                DrawLine3D(mesh.vertices[ia], mesh.vertices[ib], GREEN);
                DrawLine3D(mesh.vertices[ib], mesh.vertices[ic], GREEN);
                DrawLine3D(mesh.vertices[ic], mesh.vertices[ia], GREEN);
            }
        }
    }
}

void DrawPersonDebugCapsule(const PersonState& person, const PersonConfig& config) {
    Vector3 capsuleBottom = Vector3Add(
        person.position,
        Vector3{0.0f, config.capsuleRadius, 0.0f});
    Vector3 capsuleTop = Vector3Add(
        person.position,
        Vector3{0.0f, config.capsuleRadius + config.capsuleHeight, 0.0f});
    DrawCapsuleWires(
        capsuleBottom,
        capsuleTop,
        config.capsuleRadius,
        12,
        6,
        person.grounded ? GREEN : RED);

    Vector3 eyePosition = Vector3Add(person.position, Vector3{0.0f, config.eyeHeight, 0.0f});
    DrawSphere(eyePosition, 0.05f, BLUE);
    DrawLine3D(eyePosition, Vector3Add(eyePosition, Vector3Scale(GetPersonCameraForward(person), 0.8f)), BLUE);
}

struct DebugAxisGizmoAxis {
    const char* label;
    Vector3 direction;
    Color color;
    float depth;
    Vector2 screenEnd;
};

void DrawDebugAxisGizmo(const Camera& camera) {
    const float gizmoSize = 82.0f;
    const float axisLength = 31.0f;
    const float headRadius = 9.0f;
    const Vector2 center = Vector2{
        static_cast<float>(GetScreenWidth()) - gizmoSize,
        92.0f};

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    DebugAxisGizmoAxis axes[3] = {
        DebugAxisGizmoAxis{"X", Vector3{1.0f, 0.0f, 0.0f}, Color{235, 70, 80, 255}, 0.0f, Vector2Zero()},
        DebugAxisGizmoAxis{"Y", Vector3{0.0f, 1.0f, 0.0f}, Color{120, 220, 50, 255}, 0.0f, Vector2Zero()},
        DebugAxisGizmoAxis{"Z", Vector3{0.0f, 0.0f, 1.0f}, Color{70, 150, 240, 255}, 0.0f, Vector2Zero()}
    };

    for (int i = 0; i < 3; ++i) {
        axes[i].depth = Vector3DotProduct(axes[i].direction, forward);
        axes[i].screenEnd = Vector2{
            center.x + Vector3DotProduct(axes[i].direction, right) * axisLength,
            center.y - Vector3DotProduct(axes[i].direction, up) * axisLength};
    }

    std::sort(
        axes,
        axes + 3,
        [](const DebugAxisGizmoAxis& a, const DebugAxisGizmoAxis& b) {
            return a.depth < b.depth;
        });

    DrawCircleV(center, 4.0f, Color{210, 210, 210, 210});
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), axisLength, Color{120, 120, 120, 90});

    for (int i = 0; i < 3; ++i) {
        const DebugAxisGizmoAxis& axis = axes[i];
        DrawLineEx(center, axis.screenEnd, 3.0f, axis.color);
        DrawCircleV(axis.screenEnd, headRadius, axis.color);
        DrawCircleLines(static_cast<int>(axis.screenEnd.x), static_cast<int>(axis.screenEnd.y), headRadius, Color{230, 230, 230, 180});
        int textWidth = MeasureText(axis.label, 14);
        DrawText(
            axis.label,
            static_cast<int>(axis.screenEnd.x - textWidth * 0.5f),
            static_cast<int>(axis.screenEnd.y - 7.0f),
            14,
            RAYWHITE);
    }
}
