#ifndef PHYSICS_PHYSICS_MATERIAL_HPP
#define PHYSICS_PHYSICS_MATERIAL_HPP

#include <string>

namespace physics {

struct PhysicsMaterial {
    std::string name;
    float friction;
    float restitution;

    PhysicsMaterial()
        : name("default"),
          friction(0.5f),
          restitution(0.0f) {}
};

}  // namespace physics

#endif
