#include "gameplayRender.hpp"

#include <raylib.h>

#include "debug/levelDebugDraw.hpp"
#include "editor/editorDraw.hpp"
#include "entity/entityRegistry.hpp"
#include "level/levelLoader.hpp"
#include "personController.hpp"
#include "render/fogRenderer.hpp"

void DrawGameplay(GameWorld& gameWorld) {
    BeginMode3D(gameWorld.render.camera);
    DrawLevelSkybox(gameWorld.world.level, gameWorld.render.camera);
    UpdateFogShader(gameWorld.render.fogShader, gameWorld.world.runtimeConfig.fog, gameWorld.render.camera);
    UpdateLightingShader(gameWorld.render.fogShader, gameWorld.world.runtimeConfig.lighting);
    DrawLevel(gameWorld.world.level);
    DrawEntityCharacters(gameWorld.npcs);
    if (gameWorld.debugUi.showForces) {
        DrawLevelCollidersDebug(gameWorld.world.level);
        DrawPersonDebugCapsule(gameWorld.player.state, gameWorld.player.config);
        DrawSphere(gameWorld.player.state.position, 0.05f, gameWorld.player.state.grounded ? GREEN : RED);
    }
    editor::Draw3DOverlays(gameWorld);
    EndMode3D();
    editor::Draw2DOverlays(gameWorld);
    DrawInteractionUi(gameWorld.npcs);
    if (gameWorld.debugUi.enabled) {
        DrawDebugAxisGizmo(gameWorld.render.camera);
    }
}
