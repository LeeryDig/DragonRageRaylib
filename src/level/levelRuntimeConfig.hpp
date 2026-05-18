#ifndef LEVEL_LEVEL_RUNTIME_CONFIG_HPP
#define LEVEL_LEVEL_RUNTIME_CONFIG_HPP

#include <string>
#include <vector>

#include "interactionSystem.hpp"

struct LevelRuntimeConfig {
    std::string skyboxPath;
    std::vector<CharacterSpawnConfig> characters;
};

LevelRuntimeConfig LoadLevelRuntimeConfig(const std::string& configPath);
bool SaveLevelRuntimeConfig(const std::string& configPath, const LevelRuntimeConfig& config);

#endif
