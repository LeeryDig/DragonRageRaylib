#ifndef PHYSICS_CONTACT_HPP
#define PHYSICS_CONTACT_HPP

#include <cstddef>

#include "raylib.h"

namespace physics {

enum class PhysicsEventType {
    CollisionEnter,
    CollisionStay,
    CollisionExit,
    TriggerEnter,
    TriggerStay,
    TriggerExit
};

struct ContactPoint {
    Vector3 position;
    Vector3 normal;
    float penetrationDepth;
    Vector3 relativeVelocity;

    ContactPoint()
        : position{0.0f, 0.0f, 0.0f},
          normal{0.0f, 1.0f, 0.0f},
          penetrationDepth(0.0f),
          relativeVelocity{0.0f, 0.0f, 0.0f} {}
};

struct BodyContact {
    std::size_t selfBodyId;
    std::size_t otherBodyId;
    ContactPoint point;

    BodyContact()
        : selfBodyId(0),
          otherBodyId(0),
          point() {}
};

struct PhysicsEvent {
    PhysicsEventType type;
    std::size_t sourceId;
    std::size_t otherId;

    PhysicsEvent()
        : type(PhysicsEventType::CollisionStay),
          sourceId(0),
          otherId(0) {}
};

}  // namespace physics

#endif
