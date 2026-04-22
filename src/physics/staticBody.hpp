#ifndef PHYSICS_STATIC_BODY_HPP
#define PHYSICS_STATIC_BODY_HPP

#include "physicsBody.hpp"

namespace physics {

struct StaticBodyDesc : PhysicsBodyDesc {
    StaticBodyDesc() : PhysicsBodyDesc() {}
};

class StaticBody : public PhysicsBody {
  public:
    StaticBody(std::size_t idIn, const StaticBodyDesc& desc);

    bool IsDynamic() const override;
    void StepMotion(float deltaTime, const Vector3& gravity) override;
};

}  // namespace physics

#endif
