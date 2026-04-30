#ifndef PHYSICS_SHAPES_CAPSULE_SHAPE_HPP
#define PHYSICS_SHAPES_CAPSULE_SHAPE_HPP

#include "../shape.hpp"

namespace physics {

class CapsuleShape : public Shape {
  public:
    CapsuleShape(float radiusIn, float heightIn)
        : radius(radiusIn),
          height(heightIn) {}

    ShapeType GetType() const override {
        return ShapeType::Capsule;
    }

    std::vector<Vector3> GetLocalContactPoints() const override {
        float halfCylinder = height * 0.5f;
        std::vector<Vector3> points;
        points.reserve(10);
        points.push_back(Vector3{0.0f, halfCylinder + radius, 0.0f});
        points.push_back(Vector3{0.0f, -halfCylinder - radius, 0.0f});
        for (int y = -1; y <= 1; y += 2) {
            float centerY = halfCylinder * static_cast<float>(y);
            points.push_back(Vector3{radius, centerY, 0.0f});
            points.push_back(Vector3{-radius, centerY, 0.0f});
            points.push_back(Vector3{0.0f, centerY, radius});
            points.push_back(Vector3{0.0f, centerY, -radius});
        }
        return points;
    }

    float radius;
    float height;
};

}  // namespace physics

#endif
