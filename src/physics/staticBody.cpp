#include "staticBody.hpp"

namespace physics {

StaticBody::StaticBody(std::size_t idIn, const StaticBodyDesc& desc)
    : PhysicsBody(idIn, PhysicsBodyType::Static, desc) {}

bool StaticBody::IsDynamic() const {
    return false;
}

void StaticBody::StepMotion(float deltaTime, const Vector3& gravity) {
    (void)deltaTime;
    (void)gravity;
}

}  // namespace physics
