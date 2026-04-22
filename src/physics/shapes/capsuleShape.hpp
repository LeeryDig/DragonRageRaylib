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

    float radius;
    float height;
};

}  // namespace physics

#endif
