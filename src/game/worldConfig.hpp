#ifndef GAME_WORLD_CONFIG_HPP
#define GAME_WORLD_CONFIG_HPP

#include <string>

struct WorldConfig {
    int windowWidth;
    int windowHeight;
    std::string windowTitle;
    int targetFps;
};

WorldConfig DefaultWorldConfig();
WorldConfig LoadWorldConfig(const std::string& filePath, const WorldConfig& fallback);

#endif
