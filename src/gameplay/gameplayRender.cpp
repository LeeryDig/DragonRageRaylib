#include "gameplayRender.hpp"

#include <raylib.h>

#include "audio/radioSystem.hpp"
#include "debug/debugIcons.hpp"
#include "debug/ui/personPanel.hpp"
#include "gameplay/props.hpp"
#include "particles/particleSystem.hpp"
#include "smoking/smokingUi.hpp"
#include "uiText.hpp"

#include "debug/levelDebugDraw.hpp"
#include "editor/editorDraw.hpp"
#include "entity/entityRegistry.hpp"
#include "interaction/interactionRay.hpp"
#include "level/levelLoader.hpp"
#include "personController.hpp"
#include "render/fogRenderer.hpp"

void DrawGameplay(GameWorld& gameWorld) {
    BeginMode3D(gameWorld.render.camera);
    DrawLevelSkybox(gameWorld.world.level, gameWorld.render.camera);
    UpdateFogShader(gameWorld.render.fogShader, gameWorld.world.runtimeConfig.fog, gameWorld.render.camera);
    UpdateLightingShader(gameWorld.render.fogShader, gameWorld.world.runtimeConfig.lighting);
    DrawLevel(gameWorld.world.level);
    DrawRuntimeProps(gameWorld.world.props);
    DrawRuntimeRadios(gameWorld.world.radios);
    DrawEntityCharacters(gameWorld.npcs);
    if (gameWorld.debugUi.showForces) {
        DrawLevelCollidersDebug(gameWorld.world.level);
        DrawPersonDebugCapsule(gameWorld.player.state, gameWorld.player.config);
        DrawSphere(gameWorld.player.state.position, 0.05f, gameWorld.player.state.grounded ? GREEN : RED);
        DrawInteractionRayDebug(gameWorld.render.camera, gameWorld.player.config.interactionRayLength);
    }
    gameWorld.particles.DrawParticles(gameWorld.render.camera);
    editor::Draw3DOverlays(gameWorld);
    EndMode3D();
    editor::Draw2DOverlays(gameWorld);
    DrawInteractionUi(gameWorld.npcs);
    if (FindInteractableRadio(
            gameWorld.world.radios,
            gameWorld.render.camera,
            gameWorld.player.config.interactionRayLength) != nullptr) {
        DrawUiText("[E] Ligar/desligar radio", 28, GetScreenHeight() - 92, 20, YELLOW);
    }
    DrawSmokingUi(gameWorld.player.smoking, GetScreenWidth(), GetScreenHeight());
    debug_ui::DrawPersonPanel(gameWorld);
    if (gameWorld.debugUi.enabled) {
        DrawDebugAxisGizmo(gameWorld.render.camera);
    }
}
