#ifndef LEVEL_LEVELS_CONFIG_HPP
#define LEVEL_LEVELS_CONFIG_HPP

#include <string>
#include <vector>

struct LevelConfigEntry {
    std::string name;
    std::string path;
    std::string configPath;
};

struct LevelsConfig {
    std::vector<LevelConfigEntry> levels;
};

LevelsConfig DefaultLevelsConfig();
LevelsConfig LoadLevelsConfig(const std::string& filePath, const LevelsConfig& fallbackConfig);
bool SaveLevelsConfig(const std::string& filePath, const LevelsConfig& config);
const LevelConfigEntry* GetLevelConfigEntry(const LevelsConfig& config, int index);
std::string GetLevelDisplayName(const LevelConfigEntry& entry);

#endif
