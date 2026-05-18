#ifndef PHYSICS_SHAPES_BOX_SHAPE_HPP
#define PHYSICS_SHAPES_BOX_SHAPE_HPP

#include "raymath.h"

#include "../shape.hpp"

namespace physics {

class BoxShape : public Shape {
  public:
    explicit BoxShape(const Vector3& sizeIn)
        : size(sizeIn) {}

    ShapeType GetType() const override {
        return ShapeType::Box;
    }

    std::vector<Vector3> GetLocalContactPoints() const override {
        Vector3 half = Vector3Scale(size, 0.5f);
        std::vector<Vector3> points;
        points.reserve(8);
        for (int x = -1; x <= 1; x += 2) {
            for (int y = -1; y <= 1; y += 2) {
                for (int z = -1; z <= 1; z += 2) {
                    points.push_back(Vector3{
                        half.x * static_cast<float>(x),
                        half.y * static_cast<float>(y),
                        half.z * static_cast<float>(z)
                    });
                }
            }
        }
        return points;
    }

    Vector3 size;
};

}  // namespace physics

#endif
