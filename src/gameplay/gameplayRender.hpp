#ifndef GAMEPLAY_GAMEPLAY_RENDER_HPP
#define GAMEPLAY_GAMEPLAY_RENDER_HPP

#include "game/gameWorld.hpp"

struct GameplayRenderContext {
    WorldContext& world;
    PlayerContext& player;
    RenderContext& render;
    EntityRegistry& npcs;
    DebugUiState& debugUi;
    ParticleSystem& particles;
    GameWorld* editorWorld;
};

void DrawGameplay(GameplayRenderContext context);
void DrawGameplay(GameWorld& gameWorld);

#endif
