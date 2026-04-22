#ifndef PHYSICS_COLLISION_LAYERS_HPP
#define PHYSICS_COLLISION_LAYERS_HPP

#include <cstdint>

namespace physics {

typedef std::uint32_t CollisionMask;

enum class CollisionLayer : std::uint32_t {
    None = 0,
    World = 1u << 0,
    Vehicle = 1u << 1,
    Scenery = 1u << 2,
    Trigger = 1u << 3,
    Pickup = 1u << 4,
    Sensor = 1u << 5,
    All = 0xFFFFFFFFu
};

inline CollisionMask ToMask(CollisionLayer layer) {
    return static_cast<CollisionMask>(layer);
}

inline bool MaskMatches(CollisionMask mask, CollisionLayer layer) {
    return (mask & ToMask(layer)) != 0u;
}

inline bool MasksCollide(
    CollisionMask layerA,
    CollisionMask maskA,
    CollisionMask layerB,
    CollisionMask maskB) {
    return (maskA & layerB) != 0u && (maskB & layerA) != 0u;
}

}  // namespace physics

#endif
