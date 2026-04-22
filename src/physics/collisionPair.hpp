#ifndef PHYSICS_COLLISION_PAIR_HPP
#define PHYSICS_COLLISION_PAIR_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace physics {

inline std::uint64_t MakePairKey(std::size_t a, std::size_t b) {
    std::size_t first = std::min(a, b);
    std::size_t second = std::max(a, b);
    return (static_cast<std::uint64_t>(first) << 32) | static_cast<std::uint64_t>(second);
}

inline std::uint64_t MakeOrderedPairKey(std::size_t first, std::size_t second) {
    return (static_cast<std::uint64_t>(first) << 32) | static_cast<std::uint64_t>(second);
}

}  // namespace physics

#endif
