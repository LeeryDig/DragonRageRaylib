#include "levelsConfig.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "raylib.h"

#include "assets/json.hpp"

namespace {

using assets::GetMember;
using assets::JsonParser;
using assets::JsonValue;
using assets::StringMember;

std::string EscapeJsonString(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

}  // namespace

LevelsConfig DefaultLevelsConfig() {
    LevelsConfig config;
    LevelConfigEntry entry;
    entry.name = "Teste";
    entry.path = "resources/levels/levelteste.glb";
    entry.configPath = "resources/levels/levelteste.json";
    config.levels.push_back(entry);
    return config;
}

LevelsConfig LoadLevelsConfig(const std::string& filePath, const LevelsConfig& fallbackConfig) {
    LevelsConfig config = fallbackConfig;

    char* rawFileContents = LoadFileText(filePath.c_str());
    if (rawFileContents == nullptr) return config;

    std::string json = rawFileContents;
    UnloadFileText(rawFileContents);

    JsonParser parser(json);
    JsonValue root = parser.Parse();
    if (parser.HadError()) {
        TraceLog(LOG_WARNING, "LevelsConfig: JSON parse warning in %s: %s", filePath.c_str(), parser.Error().c_str());
    }
    if (root.type != JsonValue::Object) {
        TraceLog(LOG_WARNING, "LevelsConfig: root is not object in %s", filePath.c_str());
        return config;
    }

    const JsonValue* levels = GetMember(root, "levels");
    if (!levels || levels->type != JsonValue::Array || levels->arrayValue.empty()) return config;

    config.levels.clear();
    for (std::size_t i = 0; i < levels->arrayValue.size(); ++i) {
        const JsonValue& item = levels->arrayValue[i];
        if (item.type != JsonValue::Object) continue;
        LevelConfigEntry entry;
        entry.name = StringMember(item, "name", "");
        entry.path = StringMember(item, "path", "");
        entry.configPath = StringMember(item, "config", "");
        if (!entry.path.empty()) {
            if (entry.name.empty()) entry.name = entry.path;
            config.levels.push_back(entry);
        }
    }

    return config.levels.empty() ? fallbackConfig : config;
}

bool SaveLevelsConfig(const std::string& filePath, const LevelsConfig& config) {
    std::ofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"levels\": [\n";
    for (std::size_t i = 0; i < config.levels.size(); ++i) {
        const LevelConfigEntry& entry = config.levels[i];
        file << "    {\n";
        file << "      \"name\": \"" << EscapeJsonString(entry.name) << "\",\n";
        file << "      \"path\": \"" << EscapeJsonString(entry.path) << "\",\n";
        file << "      \"config\": \"" << EscapeJsonString(entry.configPath) << "\"\n";
        file << "    }" << (i + 1 < config.levels.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    return true;
}

const LevelConfigEntry* GetLevelConfigEntry(const LevelsConfig& config, int index) {
    if (index < 0 || index >= static_cast<int>(config.levels.size())) return nullptr;
    return &config.levels[static_cast<std::size_t>(index)];
}

std::string GetLevelDisplayName(const LevelConfigEntry& entry) {
    return entry.name.empty() ? entry.path : entry.name;
}
