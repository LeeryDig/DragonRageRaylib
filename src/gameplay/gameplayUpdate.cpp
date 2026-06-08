#include "gameplayUpdate.hpp"

#include <raylib.h>
#include <raymath.h>

#include "smoking/smokingSystem.hpp"
#include "particles/particleSystem.hpp"

#include "debug/cameraDebug.hpp"
#include "entity/entityRegistry.hpp"
#include "game/gameWorld.hpp"
#include "gameState.hpp"
#include "input/inputMap.hpp"
#include "personController.hpp"
#include "physics/jolt/joltWorld.hpp"

void UpdateGameplay(GameplayUpdateContext context, float frameDelta) {
    PlayerContext& player = context.player;
    RenderContext& render = context.render;
    EntityRegistry& npcs = context.npcs;
    DebugUiState& debugUi = context.debugUi;
    ParticleSystem& particles = context.particles;

    float physicsStep = player.config.fixedTimeStep;
    player.physicsAccumulator += frameDelta;

    if (debugUi.enabled) {
        bool wantsFreeCamera = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
        if (wantsFreeCamera && !debugUi.freeCameraActive) {
            debugUi.freeCameraActive = true;
            DisableCursor();
        } else if (!wantsFreeCamera && debugUi.freeCameraActive) {
            debugUi.freeCameraActive = false;
            EnableCursor();
        }

        if (debugUi.freeCameraActive) {
            UpdateDebugCamera(render.debugCamera, render.camera);
        }
        player.physicsAccumulator = 0.0f;
        return;
    }

    bool dialogueOpen = npcs.dialogueOpen;
    if (dialogueOpen) {
        player.physicsAccumulator = 0.0f;
        UpdateDialogueInput(npcs, player.input);
        sysState = npcs.dialogueOpen ? SysState::DIALOGUE : SysState::PLAYING;
        ApplyPersonCamera(render.camera, player.state, player.config, frameDelta);
        return;
    }

    sysState = SysState::PLAYING;
    UpdatePersonLook(player.state, player.config);
    PersonInput input = ReadPersonInput(player.input, true);

    int steps = 0;
    while (player.physicsAccumulator >= physicsStep && steps < 8) {
        UpdatePersonHorizontalMovement(player.state, player.config, input, physicsStep);

        Vector3 horizontalVelocity = Vector3{
            player.state.velocity.x, 0.0f, player.state.velocity.z};
        player.physics->UpdateCharacter(
            player.state, player.config, horizontalVelocity, physicsStep);
        bool playerMovedByNpcCollision = ResolveCharacterCollisions(
            npcs, player.state.position, player.config.capsuleRadius);
        if (playerMovedByNpcCollision) {
            player.physics->SetCharacterPosition(player.state.position);
        }

        player.physicsAccumulator -= physicsStep;
        ++steps;
    }

    ApplyPersonCamera(render.camera, player.state, player.config, frameDelta);
    UpdateSmoking(player.smoking, player.smokingConfig, player.input, frameDelta);
    UpdateSmokingParticles(player.smoking, player.smokingConfig, particles, player.state.position);
    particles.UpdateParticles(frameDelta);
    UpdateEntityFocus(npcs, render.camera, player.config.interactionRayLength);
    if (IsActionPressed(player.input, GameAction::Interact)) {
        BeginFocusedDialogue(npcs);
        if (npcs.dialogueOpen) {
            sysState = SysState::DIALOGUE;
        }
    }
}

void UpdateGameplay(GameWorld& gameWorld, float frameDelta) {
    UpdateGameplay(
        GameplayUpdateContext{
            gameWorld.player,
            gameWorld.render,
            gameWorld.npcs,
            gameWorld.debugUi,
            gameWorld.particles},
        frameDelta);
}
