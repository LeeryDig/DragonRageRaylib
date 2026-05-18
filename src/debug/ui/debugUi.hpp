#ifndef DEBUG_UI_DEBUG_UI_HPP
#define DEBUG_UI_DEBUG_UI_HPP

#include <functional>

#include "game/gameWorld.hpp"

namespace debug_ui {

struct TopBarActions {
    std::function<void(GameWorld&)> restartLevel;
    std::function<void(GameWorld&)> resetGameWorld;
};

struct LevelConfigActions {
    std::function<void(GameWorld&)> saveCurrentLevelRuntimeConfig;
    std::function<void(GameWorld&)> reloadCurrentLevelForConfig;
    std::function<void(GameWorld&, int)> loadConfiguredLevel;
    std::function<void(GameWorld&, int, int)> moveLevelConfigEntry;
};

void DrawTopBar(GameWorld& gameWorld, const TopBarActions& actions);
void DrawLevelConfigSidebar(GameWorld& gameWorld, const LevelConfigActions& actions);

}  // namespace debug_ui

#endif
