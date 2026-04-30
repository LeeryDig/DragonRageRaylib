#ifndef PHYSICS_SHAPES_COMPOUND_SHAPE_HPP
#define PHYSICS_SHAPES_COMPOUND_SHAPE_HPP

#include <vector>

#include "../shape.hpp"
#include "raymath.h"

namespace physics {

class CompoundShape : public Shape {
  public:
    CompoundShape() : children() {}

    ShapeType GetType() const override {
        return ShapeType::Compound;
    }

    std::vector<Vector3> GetLocalContactPoints() const override {
        std::vector<Vector3> points;
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (!children[i].shape) {
                continue;
            }
            std::vector<Vector3> childPoints = children[i].shape->GetLocalContactPoints();
            for (std::size_t pointIndex = 0; pointIndex < childPoints.size(); ++pointIndex) {
                points.push_back(Vector3Add(
                    children[i].localTransform.position,
                    Vector3RotateByQuaternion(
                        childPoints[pointIndex],
                        children[i].localTransform.rotation)));
            }
        }
        return points;
    }

    void AddChild(const CompoundChildShape& child) {
        children.push_back(child);
    }

    const std::vector<CompoundChildShape>& GetChildren() const {
        return children;
    }

  private:
    std::vector<CompoundChildShape> children;
};

}  // namespace physics

#endif
