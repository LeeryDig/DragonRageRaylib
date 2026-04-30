#ifndef PHYSICS_SHAPE_HPP
#define PHYSICS_SHAPE_HPP

#include <memory>
#include <vector>

#include "raylib.h"

#include "transform3d.hpp"

namespace physics {

enum class ShapeType {
    Box,
    Sphere,
    Capsule,
    Compound,
    Mesh
};

class Shape {
  public:
    virtual ~Shape() {}
    virtual ShapeType GetType() const = 0;
    virtual std::vector<Vector3> GetLocalContactPoints() const {
        return std::vector<Vector3>();
    }
};

struct CompoundChildShape {
    Transform3D localTransform;
    std::shared_ptr<const Shape> shape;

    CompoundChildShape()
        : localTransform(Transform3D::Identity()),
          shape() {}
};

}  // namespace physics

#endif
