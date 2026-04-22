#ifndef PHYSICS_SHAPES_COMPOUND_SHAPE_HPP
#define PHYSICS_SHAPES_COMPOUND_SHAPE_HPP

#include <vector>

#include "../shape.hpp"

namespace physics {

class CompoundShape : public Shape {
  public:
    CompoundShape() : children() {}

    ShapeType GetType() const override {
        return ShapeType::Compound;
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
