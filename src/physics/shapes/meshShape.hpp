#ifndef PHYSICS_SHAPES_MESH_SHAPE_HPP
#define PHYSICS_SHAPES_MESH_SHAPE_HPP

#include <string>
#include <vector>

#include "../shape.hpp"

namespace physics {

class MeshShape : public Shape {
  public:
    MeshShape(const std::vector<Vector3>& verticesIn, const std::vector<unsigned int>& indicesIn, const std::string& nameIn = "")
        : vertices(verticesIn),
          indices(indicesIn),
          name(nameIn) {}

    explicit MeshShape(const std::string& assetPathIn)
        : vertices(),
          indices(),
          name(assetPathIn) {}

    ShapeType GetType() const override {
        return ShapeType::Mesh;
    }

    const std::vector<Vector3>& GetVertices() const {
        return vertices;
    }

    const std::vector<unsigned int>& GetIndices() const {
        return indices;
    }

    const std::string& GetName() const {
        return name;
    }

    const std::string& GetAssetPath() const {
        return name;
    }

  private:
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
    std::string name;
};

}  // namespace physics

#endif
