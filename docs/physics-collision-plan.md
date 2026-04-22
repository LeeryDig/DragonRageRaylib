# Physics And Collision Plan

## Goal

Build modular 3D physics and collision system for `DragonRageRaylib` inspired by Godot patterns used in `SimpleRacingGame`, but owned by this project.

System must:

- support `RigidBody` style simulation
- support `StaticBody` for track, walls, props, scenery
- support trigger-only volumes like Godot `Area3D`
- avoid hardcode in car, track, or any specific gameplay object
- expose generic API usable by car, track, pickups, checkpoints, boost pads, obstacles, and future systems
- separate rendering, gameplay, physics, collision, and trigger logic

## Source Reference

Reference behavior comes from `SimpleRacingGame`, mainly:

- `SimpleRacingGame/vehicle/vehicle.gd`
- `SimpleRacingGame/root.tscn`
- `SimpleRacingGame/track/sector.gd`
- `SimpleRacingGame/track/boost.gd`

Godot pattern we want to emulate:

- `RigidBody3D` for dynamic body
- `CollisionShape3D` for shape data
- `Area3D` for overlap-only trigger
- grounded state from contact normals, not hardcoded world Y
- gameplay objects attach to physics objects instead of embedding physics directly in scene-specific code

## Non Goals

Do not rebuild old system:

- no infinite ground plane as physics source of truth
- no invisible side walls based on `roadHalfWidth`
- no collision rules hardcoded to car or track
- no direct dependency on render mesh for gameplay logic unless wrapped as collider asset

## Design Principles

1. Physics world owns simulation state.
2. Bodies own motion state.
3. Colliders own shape and material data.
4. Gameplay controllers apply forces and read contacts.
5. Triggers detect overlap but never generate physical pushback.
6. Car and track must register into same generic system.
7. Any object should be able to become physical by composition, not inheritance from scene-specific code.

## High Level Architecture

### Core modules

- `PhysicsWorld`
- `PhysicsBody`
- `RigidBody`
- `StaticBody`
- `TriggerArea`
- `Collider`
- `Shape`
- `PhysicsMaterial`
- `CollisionLayerMask`
- `ContactManifold`
- `PhysicsEvents`

### Separation of concerns

- render layer draws models only
- physics layer simulates bodies and resolves collisions
- gameplay layer reads input and calls physics API
- triggers emit events to gameplay systems
- scene setup registers bodies and colliders into `PhysicsWorld`

## Proposed Folder Structure

```text
src/
  physics/
    physicsWorld.hpp
    physicsWorld.cpp
    physicsBody.hpp
    physicsBody.cpp
    rigidBody.hpp
    rigidBody.cpp
    staticBody.hpp
    staticBody.cpp
    triggerArea.hpp
    triggerArea.cpp
    collider.hpp
    collider.cpp
    shape.hpp
    physicsMaterial.hpp
    collisionLayers.hpp
    contact.hpp
    collisionPair.hpp
    broadphase.hpp
    narrowphase.hpp
    solver.hpp
    shapes/
      boxShape.hpp
      sphereShape.hpp
      capsuleShape.hpp
      compoundShape.hpp
      meshShape.hpp
```

## Main Runtime Objects

### PhysicsWorld

Responsibilities:

- store all bodies, colliders, triggers
- advance simulation with `Step(dt)`
- run broadphase
- run narrowphase
- solve penetrations and impulses
- update contact manifolds
- dispatch collision and trigger events

Should expose:

- create/destroy bodies
- create/destroy triggers
- register colliders
- query raycasts later
- query contacts from body

### PhysicsBody

Base object for simulation participants.

Shared data:

- transform
- linear velocity
- angular velocity
- layer/mask
- list of colliders
- material override optional
- user data or owner handle

Body modes:

- `StaticBody`
- `RigidBody`
- future `KinematicBody` optional

### RigidBody

Dynamic simulated object.

Responsibilities:

- mass, inverse mass
- inertia tensor
- force accumulation
- torque accumulation
- sleep state later
- integration step

Should expose:

- `ApplyForce`
- `ApplyForceAtPoint`
- `ApplyTorque`
- `ApplyImpulse`
- `SetLinearVelocity`
- `GetContacts`
- `IsGrounded` through contact query, not special-case body code

### StaticBody

Non-moving collider owner for:

- track surface
- walls
- buildings
- barriers
- scenery blockers

### TriggerArea

Overlap-only volume like Godot `Area3D`.

Responsibilities:

- detect enter/stay/exit
- never resolve physics impulses
- filter through layers/masks

Use cases:

- finish line
- sector split
- boost pad
- pickup
- hazard zone

## Collider Model

Each body or trigger can own one or many colliders.

Collider data:

- local transform
- shape reference
- physics material
- is trigger flag optional if collider-level triggers are desired later
- collision layer
- collision mask

This allows:

- car body with one box collider
- track with compound colliders
- checkpoint trigger with one box collider
- building with box or mesh collider

## Shapes

Initial shape set:

- `BoxShape`
- `SphereShape`
- `CapsuleShape`

Second wave:

- `CompoundShape`
- `MeshShape`

Recommendation:

- start track with `CompoundShape` or multiple `BoxShape` pieces
- add `MeshShape` only after world pipeline is stable

Reason:

- easier to debug
- faster to iterate
- avoids early complexity in narrowphase and contact generation

## Physics Material

Need generic material object similar to engine physics material.

Fields:

- `friction`
- `restitution`
- combine rule later if needed

Material must live outside car and track code.

Examples:

- asphalt
- grass
- wall
- boost pad trigger uses no physical response

## Collision Filtering

Need layer + mask system from start.

Suggested initial layers:

- `World`
- `Vehicle`
- `Scenery`
- `Trigger`
- `Pickup`
- `Sensor`

Examples:

- car body collides with `World` and `Scenery`
- trigger detects `Vehicle` but not `World`
- scenery may ignore scenery-to-scenery

## Contact Model

Need contact structure reusable by all gameplay code.

Contact data should include:

- body A
- body B
- world point
- world normal
- penetration depth
- relative velocity
- material pair data if needed later

Need persistent manifolds so we can tell:

- collision enter
- collision stay
- collision exit

## Grounded Detection

Grounded must come from contacts, like Godot pattern in `vehicle.gd`.

Rule:

- body is grounded if any current contact normal points sufficiently upward
- example threshold: `normal.y > 0.5`

This replaces old fake logic:

- no `position.y <= groundY`
- no floor snap from hardcoded plane

## Car Integration

Car should become:

- one generic `RigidBody`
- one `CarController` gameplay component using physics API

`CarController` responsibilities:

- read input
- calculate engine, brake, grip, steering forces
- apply them to `RigidBody`
- inspect contacts for grounded state
- query contact normals or wheel support later

Important:

- `CarController` must not own collision system
- `RigidBody` must not know it is a car

## Track Integration

Track should become:

- one or many `StaticBody` instances
- one or many colliders
- optional trigger volumes for sectors, finish line, boosts

Track should not contain special-case car logic.

Examples:

- asphalt surface collider
- wall collider
- guardrail collider
- trigger boxes for checkpoints

## API Sketch

```cpp
PhysicsWorld physicsWorld;

RigidBody* carBody = physicsWorld.CreateRigidBody(rigidBodyDesc);
carBody->AddCollider(BoxShape{2.0f, 0.5f, 4.0f}, carMaterial);

StaticBody* trackBody = physicsWorld.CreateStaticBody(staticBodyDesc);
trackBody->AddCollider(CompoundShape::FromBoxes(trackBoxes), asphaltMaterial);

TriggerArea* finishTrigger = physicsWorld.CreateTriggerArea(triggerDesc);
finishTrigger->AddCollider(BoxShape{26.0f, 1.0f, 14.0f});
finishTrigger->SetLayer(CollisionLayer::Trigger);
finishTrigger->SetMask(CollisionLayer::Vehicle);
```

## Simulation Flow

Per frame:

1. gameplay systems push inputs or forces into bodies
2. `PhysicsWorld::Step(dt)` runs
3. integrate external forces
4. broadphase finds possible overlap pairs
5. narrowphase builds contacts
6. solver resolves penetration and velocity response
7. contact cache updates enter/stay/exit
8. trigger overlaps dispatch events
9. gameplay reads updated body states and contacts

## Broadphase Plan

Initial option:

- naive pair checks for low object count

Next upgrade:

- AABB broadphase
- sweep and prune or uniform grid

Recommendation:

- start naive if object count is tiny
- architect interface so broadphase can be swapped later without touching gameplay code

## Narrowphase Plan

Initial support:

- box vs box
- sphere vs box
- sphere vs sphere

Later:

- capsule support
- compound support
- mesh vs primitive for static world

Need narrowphase isolated behind dedicated module.

## Solver Plan

Initial solver:

- positional correction for penetration
- impulse resolution along collision normal
- friction impulse after normal resolution

Later:

- warm starting
- multiple solver iterations tuning
- persistent manifolds
- sleeping

## Trigger Event Plan

Need event stream:

- `OnTriggerEnter`
- `OnTriggerStay`
- `OnTriggerExit`
- `OnCollisionEnter`
- `OnCollisionStay`
- `OnCollisionExit`

Gameplay systems subscribe to events or poll queues from `PhysicsWorld`.

## Debugging Plan

Need debug visualization from start.

Debug draw should render:

- collider bounds
- contact points
- contact normals
- trigger volumes
- sleeping state later
- body centers of mass later

Without this, physics iteration becomes slow and blind.

## Implementation Strategy

### Phase 1: Foundation

Build:

- `PhysicsWorld`
- `PhysicsBody`
- `RigidBody`
- `StaticBody`
- `Collider`
- `Shape`
- `PhysicsMaterial`
- layer/mask support

Goal:

- generic API exists
- no car-specific code inside physics core

### Phase 2: Primitive Collision

Build:

- box and sphere shapes
- broadphase interface
- narrowphase for primitive pairs
- basic solver

Goal:

- generic body-to-body collision works

### Phase 3: Contacts And Grounded

Build:

- contact cache
- enter/stay/exit tracking
- grounded query through contact normals

Goal:

- replace fake grounded logic

### Phase 4: Trigger System

Build:

- `TriggerArea`
- trigger overlap events

Goal:

- support finish line, sectors, boost pads, pickups

### Phase 5: Car Migration

Build:

- `CarController`
- force application via `RigidBody`
- grounded from contacts

Goal:

- car gameplay runs on top of generic physics

### Phase 6: Track Migration

Build:

- static colliders for track
- barriers
- guardrails
- offroad collision if desired

Goal:

- remove remaining legacy assumptions from world

### Phase 7: Advanced Shapes

Build:

- `CompoundShape`
- `MeshShape`

Goal:

- accurate complex world geometry without hardcoding

## Integration Option: Custom API Over External Backend

If we want robust behavior faster, best long-term route may be:

- keep project-owned API
- plug external backend under it

Example backend candidates:

- Jolt Physics
- ReactPhysics3D

Important:

- gameplay code talks only to our wrappers
- backend library remains implementation detail

This gives:

- robust collision and solver quality
- modular engine-facing API
- freedom to swap backend later

## Recommendation

Best practical route:

1. design project-owned physics API now
2. implement minimal internal version for primitives and triggers
3. keep backend abstraction clean
4. decide later whether to continue custom solver or plug Jolt under same API

## First Deliverable Recommended

First coding milestone should not be full mesh collision.

It should be:

- `src/physics/` scaffold
- body/collider/material/layer types
- primitive collision MVP
- contact-based grounded query
- trigger MVP

After that:

- migrate car
- migrate track
- then add complex shapes

## Success Criteria

System is successful when:

- car uses generic `RigidBody`
- track uses generic `StaticBody`
- checkpoints and boosts use generic `TriggerArea`
- grounded state comes from contacts
- no collision rule depends on hardcoded plane or invisible lane walls
- future objects can join physics by registration, not by custom per-object hacks
