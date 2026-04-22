#ifndef PHYSICS_TRANSFORM3D_HPP
#define PHYSICS_TRANSFORM3D_HPP

#include "raylib.h"

namespace physics {

struct Transform3D {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;

    Transform3D()
        : position{0.0f, 0.0f, 0.0f},
          rotation{0.0f, 0.0f, 0.0f, 1.0f},
          scale{1.0f, 1.0f, 1.0f} {}

    static Transform3D Identity() {
        return Transform3D();
    }
};

}  // namespace physics

#endif
