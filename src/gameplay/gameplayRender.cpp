#include "gameplayRender.hpp"

#include <raylib.h>

#include "debug/debugIcons.hpp"
#include "debug/ui/personPanel.hpp"
#include "gameplay/props.hpp"
#include "particles/particleSystem.hpp"
#include "smoking/smokingUi.hpp"

#include "debug/levelDebugDraw.hpp"
#include "editor/editorDraw.hpp"
#include "entity/entityRegistry.hpp"
#include "level/levelLoader.hpp"
#include "personController.hpp"
#include "render/fogRenderer.hpp"

void DrawGameplay(GameplayRenderContext context) {
    WorldContext& world = context.world;
    PlayerContext& player = context.player;
    RenderContext& render = context.render;
    EntityRegistry& npcs = context.npcs;
    DebugUiState& debugUi = context.debugUi;
    ParticleSystem& particles = context.particles;

    BeginMode3D(render.camera);
    DrawLevelSkybox(world.level, render.camera);
    UpdateFogShader(render.fogShader, world.runtimeConfig.fog, render.camera);
    UpdateLightingShader(render.fogShader, world.runtimeConfig.lighting);
    DrawLevel(world.level);
    DrawRuntimeProps(world.props);
    DrawEntityCharacters(npcs);
    if (debugUi.showForces) {
        DrawLevelCollidersDebug(world.level);
        DrawPersonDebugCapsule(player.state, player.config);
        DrawSphere(player.state.position, 0.05f, player.state.grounded ? GREEN : RED);
    }
    particles.DrawParticles(render.camera);
    if (context.editorWorld) {
        editor::Draw3DOverlays(*context.editorWorld);
    }
    EndMode3D();
    if (context.editorWorld) {
        editor::Draw2DOverlays(*context.editorWorld);
    }
    DrawInteractionUi(npcs);
    DrawSmokingUi(player.smoking, GetScreenWidth(), GetScreenHeight());
    if (context.editorWorld) {
        debug_ui::DrawPersonPanel(*context.editorWorld);
    }
    if (debugUi.enabled) {
        DrawDebugAxisGizmo(render.camera);
    }
}

void DrawGameplay(GameWorld& gameWorld) {
    DrawGameplay(
        GameplayRenderContext{
            gameWorld.world,
            gameWorld.player,
            gameWorld.render,
            gameWorld.npcs,
            gameWorld.debugUi,
            gameWorld.particles,
            &gameWorld});
}
