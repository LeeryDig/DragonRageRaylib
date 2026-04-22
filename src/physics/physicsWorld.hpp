#ifndef PHYSICS_PHYSICS_WORLD_HPP
#define PHYSICS_PHYSICS_WORLD_HPP

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

#include "collisionPair.hpp"
#include "contact.hpp"
#include "narrowphase.hpp"
#include "rigidBody.hpp"
#include "staticBody.hpp"
#include "triggerArea.hpp"

namespace physics {

class PhysicsWorld {
  public:
    PhysicsWorld();

    RigidBody* CreateRigidBody(const RigidBodyDesc& desc);
    StaticBody* CreateStaticBody(const StaticBodyDesc& desc);
    TriggerArea* CreateTriggerArea(const TriggerAreaDesc& desc);

    void SetGravity(const Vector3& gravityIn);
    const Vector3& GetGravity() const;

    void Step(float deltaTime);

    const std::vector<std::unique_ptr<PhysicsBody> >& GetBodies() const;
    const std::vector<std::unique_ptr<TriggerArea> >& GetTriggerAreas() const;
    const std::vector<PhysicsEvent>& GetEvents() const;

    void ClearEvents();
    void QueueEvent(const PhysicsEvent& event);

  private:
    void EmitBodyPairEvent(PhysicsEventType type, std::size_t bodyAId, std::size_t bodyBId);
    void EmitTriggerEvent(PhysicsEventType type, std::size_t triggerId, std::size_t bodyId);

    std::size_t nextBodyId;
    std::size_t nextTriggerId;
    Vector3 gravity;
    std::vector<std::unique_ptr<PhysicsBody> > bodies;
    std::vector<std::unique_ptr<TriggerArea> > triggerAreas;
    std::vector<PhysicsEvent> events;
    std::unordered_set<std::uint64_t> previousCollisionPairs;
    std::unordered_set<std::uint64_t> previousTriggerPairs;
};

}  // namespace physics

#endif
