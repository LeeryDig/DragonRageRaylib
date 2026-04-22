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

    float radius;
};

}  // namespace physics

#endif
