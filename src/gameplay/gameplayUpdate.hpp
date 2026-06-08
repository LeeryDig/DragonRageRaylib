#ifndef GAMEPLAY_GAMEPLAY_UPDATE_HPP
#define GAMEPLAY_GAMEPLAY_UPDATE_HPP

#include "game/gameWorld.hpp"

struct GameplayUpdateContext {
    PlayerContext& player;
    RenderContext& render;
    EntityRegistry& npcs;
    DebugUiState& debugUi;
    ParticleSystem& particles;
};

void UpdateGameplay(GameplayUpdateContext context, float frameDelta);
void UpdateGameplay(GameWorld& gameWorld, float frameDelta);

#endif
