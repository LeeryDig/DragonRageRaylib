#ifndef LEVEL_LEVEL_RUNTIME_CONFIG_HPP
#define LEVEL_LEVEL_RUNTIME_CONFIG_HPP

#include <string>
#include <vector>

#include "raylib.h"

#include "interactionSystem.hpp"

enum class FogMode {
    Linear
};

struct FogConfig {
    bool enabled;
    Color color;
    float start;
    float end;
    float density;
    FogMode mode;

    FogConfig()
        : enabled(false),
          color{185, 190, 200, 255},
          start(45.0f),
          end(150.0f),
          density(1.0f),
          mode(FogMode::Linear) {}
};

struct LevelRuntimeConfig {
    std::string skyboxPath;
    FogConfig fog;
    std::vector<CharacterSpawnConfig> characters;
};

LevelRuntimeConfig LoadLevelRuntimeConfig(const std::string& configPath);
bool SaveLevelRuntimeConfig(const std::string& configPath, const LevelRuntimeConfig& config);

#endif
