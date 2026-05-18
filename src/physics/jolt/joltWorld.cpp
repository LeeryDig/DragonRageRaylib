#include "physics/jolt/joltWorld.hpp"

#include <cstdarg>
#include <cstdio>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "raymath.h"

namespace physics_jolt {

namespace {

constexpr JPH::ObjectLayer LAYER_NON_MOVING = 0;
constexpr JPH::ObjectLayer LAYER_MOVING = 1;
constexpr JPH::ObjectLayer LAYER_SENSOR = 2;
constexpr JPH::uint NUM_OBJECT_LAYERS = 3;

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr JPH::BroadPhaseLayer SENSOR(2);
static constexpr JPH::uint NUM_LAYERS = 3;
}

#ifdef JPH_ENABLE_ASSERTS
bool AssertFailedImpl(const char* expression, const char* message, const char* file, JPH::uint line) {
    TraceLog(LOG_ERROR, "Jolt assert: %s:%u (%s) %s", file, line, expression ? expression : "", message ? message : "");
    return true;
}
#endif

void TraceImpl(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    TraceLog(LOG_INFO, "Jolt: %s", buffer);
}

JPH::Vec3 ToJolt(Vector3 value) {
    return JPH::Vec3(value.x, value.y, value.z);
}

Vector3 FromJolt(JPH::Vec3 value) {
    return Vector3{value.GetX(), value.GetY(), value.GetZ()};
}

JPH::Quat ToJolt(Quaternion value) {
    return JPH::Quat(value.x, value.y, value.z, value.w);
}

JPH::RVec3 ToJoltR(Vector3 value) {
    return JPH::RVec3(value.x, value.y, value.z);
}

class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
  public:
    BroadPhaseLayerInterfaceImpl() {
        objectToBroadPhase[LAYER_NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        objectToBroadPhase[LAYER_MOVING] = BroadPhaseLayers::MOVING;
        objectToBroadPhase[LAYER_SENSOR] = BroadPhaseLayers::SENSOR;
    }

    JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override { return objectToBroadPhase[layer]; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        switch ((JPH::BroadPhaseLayer::Type)layer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::SENSOR: return "SENSOR";
            default: return "INVALID";
        }
    }
#endif

  private:
    JPH::BroadPhaseLayer objectToBroadPhase[NUM_OBJECT_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
  public:
    bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override {
        switch (layer1) {
            case LAYER_NON_MOVING:
                return layer2 == BroadPhaseLayers::MOVING;
            case LAYER_MOVING:
                return true;
            case LAYER_SENSOR:
                return layer2 == BroadPhaseLayers::MOVING;
            default:
                return false;
        }
    }
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
  public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        if (a == LAYER_NON_MOVING && b == LAYER_NON_MOVING) return false;
        if (a == LAYER_SENSOR && b == LAYER_NON_MOVING) return false;
        if (a == LAYER_NON_MOVING && b == LAYER_SENSOR) return false;
        if (a == LAYER_SENSOR && b == LAYER_SENSOR) return false;
        return true;
    }
};

}  // namespace

struct JoltWorld::Impl {
    bool ready = false;
    BroadPhaseLayerInterfaceImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem;
    std::vector<JPH::BodyID> levelBodies;
    std::unique_ptr<JPH::CharacterVirtual> character;
    JPH::RefConst<JPH::Shape> characterShape;
    Vector3 characterVelocity = Vector3{0.0f, 0.0f, 0.0f};
};

JoltWorld::JoltWorld() : impl(new Impl()) {}
JoltWorld::~JoltWorld() { Shutdown(); }

bool JoltWorld::Init() {
    if (impl->ready) return true;

    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = AssertFailedImpl;
#endif
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    impl->tempAllocator.reset(new JPH::TempAllocatorImpl(16 * 1024 * 1024));
    impl->jobSystem.reset(new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 1));
    impl->physicsSystem.reset(new JPH::PhysicsSystem());
    impl->physicsSystem->Init(
        4096,
        0,
        8192,
        2048,
        impl->broadPhaseLayerInterface,
        impl->objectVsBroadPhaseLayerFilter,
        impl->objectLayerPairFilter);
    impl->physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    impl->ready = true;
    return true;
}

void JoltWorld::Shutdown() {
    if (!impl || !impl->ready) return;
    Clear();
    impl->physicsSystem.reset();
    impl->jobSystem.reset();
    impl->tempAllocator.reset();
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    impl->ready = false;
}

void JoltWorld::Clear() {
    if (!impl->ready) return;
    JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();
    impl->character.reset();
    impl->characterShape = nullptr;
    for (JPH::BodyID id : impl->levelBodies) {
        if (!id.IsInvalid()) {
            bodyInterface.RemoveBody(id);
            bodyInterface.DestroyBody(id);
        }
    }
    impl->levelBodies.clear();
}

void JoltWorld::LoadLevel(const LevelData& level) {
    if (!impl->ready) return;
    Clear();
    JPH::BodyInterface& bodyInterface = impl->physicsSystem->GetBodyInterface();

    for (std::size_t i = 0; i < level.colliders.size(); ++i) {
        const LevelBoxVolume& box = level.colliders[i];
        JPH::Vec3 halfExtent(box.size.x * 0.5f, box.size.y * 0.5f, box.size.z * 0.5f);
        JPH::RefConst<JPH::Shape> shape = new JPH::BoxShape(halfExtent);
        JPH::BodyCreationSettings settings(shape, ToJoltR(box.position), ToJolt(box.rotation), JPH::EMotionType::Static, LAYER_NON_MOVING);
        JPH::BodyID id = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
        impl->levelBodies.push_back(id);
    }

    for (std::size_t i = 0; i < level.collisionMeshes.size(); ++i) {
        const LevelCollisionMesh& mesh = level.collisionMeshes[i];
        if (mesh.vertices.empty() || mesh.indices.size() < 3) continue;

        JPH::TriangleList triangles;
        triangles.reserve(mesh.indices.size() / 3);
        for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
            unsigned int ia = mesh.indices[t];
            unsigned int ib = mesh.indices[t + 1];
            unsigned int ic = mesh.indices[t + 2];
            if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() || ic >= mesh.vertices.size()) continue;
            triangles.push_back(JPH::Triangle(ToJolt(mesh.vertices[ia]), ToJolt(mesh.vertices[ib]), ToJolt(mesh.vertices[ic])));
        }
        if (triangles.empty()) continue;

        JPH::MeshShapeSettings meshSettings(triangles);
        JPH::ShapeSettings::ShapeResult result = meshSettings.Create();
        if (result.HasError()) {
            TraceLog(LOG_WARNING, "Jolt mesh collider failed: %s", result.GetError().c_str());
            continue;
        }

        JPH::BodyCreationSettings settings(result.Get(), JPH::RVec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Static, LAYER_NON_MOVING);
        JPH::BodyID id = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
        impl->levelBodies.push_back(id);
    }

    for (std::size_t i = 0; i < level.triggers.size(); ++i) {
        const LevelBoxVolume& trigger = level.triggers[i];
        JPH::RefConst<JPH::Shape> shape = new JPH::BoxShape(JPH::Vec3(trigger.size.x * 0.5f, trigger.size.y * 0.5f, trigger.size.z * 0.5f));
        JPH::BodyCreationSettings settings(shape, ToJoltR(trigger.position), ToJolt(trigger.rotation), JPH::EMotionType::Static, LAYER_SENSOR);
        settings.mIsSensor = true;
        JPH::BodyID id = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
        impl->levelBodies.push_back(id);
    }

    impl->physicsSystem->OptimizeBroadPhase();
}

void JoltWorld::CreateCharacter(const PersonConfig& config, Vector3 position) {
    if (!impl->ready) return;
    float halfHeight = config.capsuleHeight * 0.5f;
    impl->characterShape = new JPH::CapsuleShape(halfHeight, config.capsuleRadius);

    JPH::CharacterVirtualSettings settings;
    settings.mShape = impl->characterShape;
    settings.mMaxSlopeAngle = 50.0f * DEG2RAD;
    settings.mMaxStrength = 100.0f;
    settings.mCharacterPadding = 0.02f;
    settings.mPenetrationRecoverySpeed = 1.0f;
    settings.mPredictiveContactDistance = 0.1f;

    Vector3 center = Vector3Add(position, Vector3{0.0f, config.capsuleRadius + halfHeight, 0.0f});
    impl->character.reset(new JPH::CharacterVirtual(&settings, ToJoltR(center), JPH::Quat::sIdentity(), impl->physicsSystem.get()));
    impl->characterVelocity = Vector3{0.0f, 0.0f, 0.0f};
}

void JoltWorld::SetCharacterPosition(Vector3 position) {
    if (!impl->character) return;
    // Keep current configured capsule bottom at requested feet position approximately.
    JPH::AABox bounds = impl->characterShape->GetLocalBounds();
    float bottomToCenter = -bounds.mMin.GetY();
    impl->character->SetPosition(ToJoltR(Vector3Add(position, Vector3{0.0f, bottomToCenter, 0.0f})));
}

void JoltWorld::UpdateCharacter(PersonState& person, const PersonConfig& config, Vector3 desiredHorizontalVelocity, float deltaTime) {
    if (!impl->ready || !impl->character || deltaTime <= 0.0f) return;

    JPH::Vec3 groundVelocity = impl->character->GetGroundVelocity();
    bool grounded = impl->character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;

    Vector3 velocity = impl->characterVelocity;
    velocity.x = desiredHorizontalVelocity.x;
    velocity.z = desiredHorizontalVelocity.z;
    if (grounded && velocity.y < 0.0f) velocity.y = 0.0f;
    velocity.y -= config.gravity * deltaTime;

    impl->characterVelocity = velocity;
    impl->character->SetLinearVelocity(ToJolt(velocity) + groundVelocity);

    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    updateSettings.mStickToFloorStepDown = JPH::Vec3(0.0f, -config.groundSnapDistance, 0.0f);
    updateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, 0.35f, 0.0f);

    impl->character->ExtendedUpdate(
        deltaTime,
        impl->physicsSystem->GetGravity(),
        updateSettings,
        impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(LAYER_MOVING),
        impl->physicsSystem->GetDefaultLayerFilter(LAYER_MOVING),
        {},
        {},
        *impl->tempAllocator);

    impl->physicsSystem->Update(deltaTime, 1, impl->tempAllocator.get(), impl->jobSystem.get());

    JPH::RVec3 center = impl->character->GetPosition();
    JPH::AABox bounds = impl->characterShape->GetLocalBounds();
    float bottomToCenter = -bounds.mMin.GetY();
    person.position = Vector3{static_cast<float>(center.GetX()), static_cast<float>(center.GetY() - bottomToCenter), static_cast<float>(center.GetZ())};
    person.velocity = FromJolt(impl->character->GetLinearVelocity());
    impl->characterVelocity = person.velocity;
    person.grounded = impl->character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
}

bool JoltWorld::Raycast(Vector3 origin, Vector3 direction, float distance, RaycastHit& hit) const {
    hit = RaycastHit();
    if (!impl->ready || Vector3LengthSqr(direction) <= 0.0001f || distance <= 0.0f) return false;

    Vector3 dir = Vector3Normalize(direction);
    JPH::RRayCast ray(ToJoltR(origin), ToJolt(dir) * distance);
    JPH::RayCastResult result;
    bool didHit = impl->physicsSystem->GetNarrowPhaseQuery().CastRay(ray, result);
    if (!didHit) return false;

    hit.hit = true;
    hit.distance = result.mFraction * distance;
    hit.position = Vector3Add(origin, Vector3Scale(dir, hit.distance));

    JPH::BodyLockRead lock(impl->physicsSystem->GetBodyLockInterface(), result.mBodyID);
    if (lock.Succeeded()) {
        hit.normal = FromJolt(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ray.GetPointOnRay(result.mFraction)));
    }
    return true;
}

bool JoltWorld::IsReady() const { return impl->ready; }
bool JoltWorld::HasCharacter() const { return impl->character != nullptr; }

}  // namespace physics_jolt
