#ifndef PHYSICS_SHAPES_BOX_SHAPE_HPP
#define PHYSICS_SHAPES_BOX_SHAPE_HPP

#include "../shape.hpp"

namespace physics {

class BoxShape : public Shape {
  public:
    explicit BoxShape(const Vector3& sizeIn)
        : size(sizeIn) {}

    ShapeType GetType() const override {
        return ShapeType::Box;
    }

    Vector3 size;
};

}  // namespace physics

#endif
