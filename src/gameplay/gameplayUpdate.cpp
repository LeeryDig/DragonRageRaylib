#include "gameplayUpdate.hpp"

#include <raylib.h>
#include <raymath.h>

#include "smoking/smokingSystem.hpp"
#include "particles/particleSystem.hpp"

#include "audio/radioSystem.hpp"
#include "debug/cameraDebug.hpp"
#include "entity/entityRegistry.hpp"
#include "game/gameWorld.hpp"
#include "gameState.hpp"
#include "input/inputMap.hpp"
#include "personController.hpp"
#include "physics/jolt/joltWorld.hpp"

void UpdateGameplay(GameWorld& gameWorld, float frameDelta) {
    float physicsStep = gameWorld.player.config.fixedTimeStep;
    gameWorld.player.physicsAccumulator += frameDelta;

    if (gameWorld.debugUi.enabled) {
        bool wantsFreeCamera = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
        if (wantsFreeCamera && !gameWorld.debugUi.freeCameraActive) {
            gameWorld.debugUi.freeCameraActive = true;
            DisableCursor();
        } else if (!wantsFreeCamera && gameWorld.debugUi.freeCameraActive) {
            gameWorld.debugUi.freeCameraActive = false;
            EnableCursor();
        }

        if (gameWorld.debugUi.freeCameraActive) {
            UpdateDebugCamera(gameWorld.render.debugCamera, gameWorld.render.camera);
        }
        gameWorld.player.physicsAccumulator = 0.0f;
        return;
    }

    bool dialogueOpen = gameWorld.npcs.dialogueOpen;
    if (dialogueOpen) {
        gameWorld.player.physicsAccumulator = 0.0f;
        UpdateDialogueInput(gameWorld.npcs, gameWorld.player.input);
        sysState = gameWorld.npcs.dialogueOpen ? SysState::DIALOGUE : SysState::PLAYING;
        ApplyPersonCamera(gameWorld.render.camera, gameWorld.player.state, gameWorld.player.config, frameDelta);
        return;
    }

    sysState = SysState::PLAYING;
    UpdatePersonLook(gameWorld.player.state, gameWorld.player.config);
    PersonInput input = ReadPersonInput(gameWorld.player.input, true);

    int steps = 0;
    while (gameWorld.player.physicsAccumulator >= physicsStep && steps < 8) {
        UpdatePersonHorizontalMovement(gameWorld.player.state, gameWorld.player.config, input, physicsStep);

        Vector3 horizontalVelocity = Vector3{
            gameWorld.player.state.velocity.x, 0.0f, gameWorld.player.state.velocity.z};
        gameWorld.player.physics->UpdateCharacter(
            gameWorld.player.state, gameWorld.player.config, horizontalVelocity, physicsStep);
        ResolveCharacterCollisions(
            gameWorld.npcs, gameWorld.player.state.position, gameWorld.player.config.capsuleRadius);

        gameWorld.player.physicsAccumulator -= physicsStep;
        ++steps;
    }

    ApplyPersonCamera(gameWorld.render.camera, gameWorld.player.state, gameWorld.player.config, frameDelta);
    UpdateSmoking(gameWorld.player.smoking, gameWorld.player.smokingConfig, gameWorld.player.input, frameDelta);
    UpdateSmokingParticles(gameWorld.player.smoking, gameWorld.player.smokingConfig, gameWorld.particles, gameWorld.player.state.position);
    gameWorld.particles.UpdateParticles(frameDelta);
    UpdateEntityFocus(
        gameWorld.npcs,
        gameWorld.render.camera,
        gameWorld.player.config.interactionRayLength);
    UpdateRuntimeRadios(gameWorld.world.radios, gameWorld.player.state.position);
    if (IsActionPressed(gameWorld.player.input, GameAction::Interact)) {
        RuntimeRadio* radio = FindInteractableRadio(
            gameWorld.world.radios,
            gameWorld.render.camera,
            gameWorld.player.config.interactionRayLength);
        if (radio != nullptr) {
            ToggleRuntimeRadio(*radio);
        } else {
            BeginFocusedDialogue(gameWorld.npcs);
            if (gameWorld.npcs.dialogueOpen) {
                sysState = SysState::DIALOGUE;
            }
        }
    }
}
