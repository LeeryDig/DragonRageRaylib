#ifndef PHYSICS_SHAPES_SPHERE_SHAPE_HPP
#define PHYSICS_SHAPES_SPHERE_SHAPE_HPP

#include "../shape.hpp"

namespace physics {

class SphereShape : public Shape {
  public:
    explicit SphereShape(float radiusIn)
        : radius(radiusIn) {}

    ShapeType GetType() const override {
        return ShapeType::Sphere;
    }

    std::vector<Vector3> GetLocalContactPoints() const override {
        std::vector<Vector3> points;
        points.reserve(6);
        points.push_back(Vector3{radius, 0.0f, 0.0f});
        points.push_back(Vector3{-radius, 0.0f, 0.0f});
        points.push_back(Vector3{0.0f, radius, 0.0f});
        points.push_back(Vector3{0.0f, -radius, 0.0f});
        points.push_back(Vector3{0.0f, 0.0f, radius});
        points.push_back(Vector3{0.0f, 0.0f, -radius});
        return points;
    }

    float radius;
};

}  // namespace physics

#endif
