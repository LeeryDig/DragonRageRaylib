#include "physicsWorld.hpp"

#include <algorithm>
#include <unordered_set>

#include "raymath.h"

namespace physics {

namespace {

float DotAlongNormal(const Vector3& velocity, const Vector3& normal) {
    return Vector3DotProduct(velocity, normal);
}

void ResolveBodyOverlap(
    PhysicsBody& bodyA,
    PhysicsBody& bodyB,
    const OverlapResult& overlap) {
    if (!bodyA.IsDynamic() && !bodyB.IsDynamic()) {
        return;
    }

    Vector3 separation = Vector3Scale(overlap.point.normal, overlap.point.penetrationDepth);
    if (bodyA.IsDynamic() && bodyB.IsDynamic()) {
        bodyA.SetPosition(Vector3Subtract(bodyA.GetPosition(), Vector3Scale(separation, 0.5f)));
        bodyB.SetPosition(Vector3Add(bodyB.GetPosition(), Vector3Scale(separation, 0.5f)));
    } else if (bodyA.IsDynamic()) {
        bodyA.SetPosition(Vector3Subtract(bodyA.GetPosition(), separation));
    } else if (bodyB.IsDynamic()) {
        bodyB.SetPosition(Vector3Add(bodyB.GetPosition(), separation));
    }

    if (bodyA.IsDynamic()) {
        float normalSpeed = DotAlongNormal(bodyA.GetLinearVelocity(), overlap.point.normal);
        if (normalSpeed > 0.0f) {
            bodyA.SetLinearVelocity(
                Vector3Subtract(
                    bodyA.GetLinearVelocity(),
                    Vector3Scale(overlap.point.normal, normalSpeed)));
        }
    }

    if (bodyB.IsDynamic()) {
        Vector3 inverseNormal = Vector3Negate(overlap.point.normal);
        float normalSpeed = DotAlongNormal(bodyB.GetLinearVelocity(), inverseNormal);
        if (normalSpeed > 0.0f) {
            bodyB.SetLinearVelocity(
                Vector3Subtract(
                    bodyB.GetLinearVelocity(),
                    Vector3Scale(inverseNormal, normalSpeed)));
        }
    }
}

BodyContact BuildBodyContact(
    const PhysicsBody& selfBody,
    const PhysicsBody& otherBody,
    const OverlapResult& overlap,
    bool invertNormal) {
    BodyContact contact;
    contact.selfBodyId = selfBody.GetId();
    contact.otherBodyId = otherBody.GetId();
    contact.point = overlap.point;
    if (invertNormal) {
        contact.point.normal = Vector3Negate(contact.point.normal);
        contact.point.relativeVelocity = Vector3Negate(contact.point.relativeVelocity);
    }
    return contact;
}

}  // namespace

PhysicsWorld::PhysicsWorld()
    : nextBodyId(1),
      nextTriggerId(1),
      gravity{0.0f, -9.81f, 0.0f},
      bodies(),
      triggerAreas(),
      events(),
      previousCollisionPairs(),
      previousTriggerPairs() {}

RigidBody* PhysicsWorld::CreateRigidBody(const RigidBodyDesc& desc) {
    std::unique_ptr<RigidBody> body(new RigidBody(nextBodyId++, desc));
    RigidBody* rawBody = body.get();
    bodies.push_back(std::unique_ptr<PhysicsBody>(body.release()));
    return rawBody;
}

StaticBody* PhysicsWorld::CreateStaticBody(const StaticBodyDesc& desc) {
    std::unique_ptr<StaticBody> body(new StaticBody(nextBodyId++, desc));
    StaticBody* rawBody = body.get();
    bodies.push_back(std::unique_ptr<PhysicsBody>(body.release()));
    return rawBody;
}

TriggerArea* PhysicsWorld::CreateTriggerArea(const TriggerAreaDesc& desc) {
    std::unique_ptr<TriggerArea> area(new TriggerArea(nextTriggerId++, desc));
    TriggerArea* rawArea = area.get();
    triggerAreas.push_back(std::move(area));
    return rawArea;
}

void PhysicsWorld::SetGravity(const Vector3& gravityIn) {
    gravity = gravityIn;
}

const Vector3& PhysicsWorld::GetGravity() const {
    return gravity;
}

void PhysicsWorld::Step(float deltaTime) {
    events.clear();

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        bodies[i]->ClearContacts();
    }

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        bodies[i]->StepMotion(deltaTime, gravity);
    }

    std::unordered_set<std::uint64_t> currentCollisionPairs;
    for (std::size_t bodyAIndex = 0; bodyAIndex < bodies.size(); ++bodyAIndex) {
        PhysicsBody& bodyA = *bodies[bodyAIndex];
        for (std::size_t bodyBIndex = bodyAIndex + 1; bodyBIndex < bodies.size(); ++bodyBIndex) {
            PhysicsBody& bodyB = *bodies[bodyBIndex];

            bool pairCollided = false;
            OverlapResult overlap = {};
            for (std::size_t colliderAIndex = 0; colliderAIndex < bodyA.GetColliders().size(); ++colliderAIndex) {
                const Collider& colliderA = bodyA.GetColliders()[colliderAIndex];
                for (std::size_t colliderBIndex = 0; colliderBIndex < bodyB.GetColliders().size(); ++colliderBIndex) {
                    const Collider& colliderB = bodyB.GetColliders()[colliderBIndex];
                    if (colliderA.IsTrigger() || colliderB.IsTrigger()) {
                        continue;
                    }
                    if (!MasksCollide(
                            colliderA.GetLayer(),
                            colliderA.GetMask(),
                            colliderB.GetLayer(),
                            colliderB.GetMask())) {
                        continue;
                    }
                    if (ComputeBodyOverlap(bodyA, colliderA, bodyB, colliderB, overlap)) {
                        pairCollided = true;
                        break;
                    }
                }
                if (pairCollided) {
                    break;
                }
            }

            if (!pairCollided) {
                continue;
            }

            ResolveBodyOverlap(bodyA, bodyB, overlap);
            bodyA.AddContact(BuildBodyContact(bodyA, bodyB, overlap, true));
            bodyB.AddContact(BuildBodyContact(bodyB, bodyA, overlap, false));

            std::uint64_t pairKey = MakePairKey(bodyA.GetId(), bodyB.GetId());
            currentCollisionPairs.insert(pairKey);
            EmitBodyPairEvent(
                previousCollisionPairs.find(pairKey) == previousCollisionPairs.end()
                    ? PhysicsEventType::CollisionEnter
                    : PhysicsEventType::CollisionStay,
                bodyA.GetId(),
                bodyB.GetId());
        }
    }

    for (std::unordered_set<std::uint64_t>::const_iterator it = previousCollisionPairs.begin();
         it != previousCollisionPairs.end();
         ++it) {
        if (currentCollisionPairs.find(*it) == currentCollisionPairs.end()) {
            std::size_t bodyAId = static_cast<std::size_t>(*it >> 32);
            std::size_t bodyBId = static_cast<std::size_t>(*it & 0xFFFFFFFFu);
            EmitBodyPairEvent(PhysicsEventType::CollisionExit, bodyAId, bodyBId);
        }
    }

    std::unordered_set<std::uint64_t> currentTriggerPairs;
    for (std::size_t triggerIndex = 0; triggerIndex < triggerAreas.size(); ++triggerIndex) {
        const TriggerArea& triggerArea = *triggerAreas[triggerIndex];
        for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
            const PhysicsBody& body = *bodies[bodyIndex];

            bool pairOverlapped = false;
            OverlapResult overlap = {};
            for (std::size_t triggerColliderIndex = 0;
                 triggerColliderIndex < triggerArea.GetColliders().size();
                 ++triggerColliderIndex) {
                const Collider& triggerCollider = triggerArea.GetColliders()[triggerColliderIndex];
                for (std::size_t bodyColliderIndex = 0;
                     bodyColliderIndex < body.GetColliders().size();
                     ++bodyColliderIndex) {
                    const Collider& bodyCollider = body.GetColliders()[bodyColliderIndex];
                    if (!MasksCollide(
                            triggerCollider.GetLayer(),
                            triggerCollider.GetMask(),
                            bodyCollider.GetLayer(),
                            bodyCollider.GetMask())) {
                        continue;
                    }
                    if (ComputeTriggerBodyBoxOverlap(
                            triggerArea,
                            triggerCollider,
                            body,
                            bodyCollider,
                            overlap)) {
                        pairOverlapped = true;
                        break;
                    }
                }
                if (pairOverlapped) {
                    break;
                }
            }

            if (!pairOverlapped) {
                continue;
            }

            std::uint64_t pairKey = MakeOrderedPairKey(triggerArea.GetId(), body.GetId());
            currentTriggerPairs.insert(pairKey);
            EmitTriggerEvent(
                previousTriggerPairs.find(pairKey) == previousTriggerPairs.end()
                    ? PhysicsEventType::TriggerEnter
                    : PhysicsEventType::TriggerStay,
                triggerArea.GetId(),
                body.GetId());
        }
    }

    for (std::unordered_set<std::uint64_t>::const_iterator it = previousTriggerPairs.begin();
         it != previousTriggerPairs.end();
         ++it) {
        if (currentTriggerPairs.find(*it) == currentTriggerPairs.end()) {
            std::size_t triggerId = static_cast<std::size_t>(*it >> 32);
            std::size_t bodyId = static_cast<std::size_t>(*it & 0xFFFFFFFFu);
            EmitTriggerEvent(PhysicsEventType::TriggerExit, triggerId, bodyId);
        }
    }

    previousCollisionPairs = currentCollisionPairs;
    previousTriggerPairs = currentTriggerPairs;
}

const std::vector<std::unique_ptr<PhysicsBody> >& PhysicsWorld::GetBodies() const {
    return bodies;
}

const std::vector<std::unique_ptr<TriggerArea> >& PhysicsWorld::GetTriggerAreas() const {
    return triggerAreas;
}

const std::vector<PhysicsEvent>& PhysicsWorld::GetEvents() const {
    return events;
}

void PhysicsWorld::ClearEvents() {
    events.clear();
}

void PhysicsWorld::QueueEvent(const PhysicsEvent& event) {
    events.push_back(event);
}

void PhysicsWorld::EmitBodyPairEvent(PhysicsEventType type, std::size_t bodyAId, std::size_t bodyBId) {
    PhysicsEvent eventA;
    eventA.type = type;
    eventA.sourceId = bodyAId;
    eventA.otherId = bodyBId;
    events.push_back(eventA);

    PhysicsEvent eventB;
    eventB.type = type;
    eventB.sourceId = bodyBId;
    eventB.otherId = bodyAId;
    events.push_back(eventB);
}

void PhysicsWorld::EmitTriggerEvent(PhysicsEventType type, std::size_t triggerId, std::size_t bodyId) {
    PhysicsEvent event;
    event.type = type;
    event.sourceId = triggerId;
    event.otherId = bodyId;
    events.push_back(event);
}

}  // namespace physics
