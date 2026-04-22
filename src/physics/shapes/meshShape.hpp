#ifndef PHYSICS_SHAPES_MESH_SHAPE_HPP
#define PHYSICS_SHAPES_MESH_SHAPE_HPP

#include <string>

#include "../shape.hpp"

namespace physics {

class MeshShape : public Shape {
  public:
    explicit MeshShape(const std::string& assetPathIn)
        : assetPath(assetPathIn) {}

    ShapeType GetType() const override {
        return ShapeType::Mesh;
    }

    const std::string& GetAssetPath() const {
        return assetPath;
    }

  private:
    std::string assetPath;
};

}  // namespace physics

#endif
