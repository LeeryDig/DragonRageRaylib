#include "levelsConfig.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "raylib.h"

namespace {

std::string ExtractArrayBlock(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    std::size_t blockStart = json.find('[', keyPos);
    if (blockStart == std::string::npos) return "";

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = blockStart; i < json.size(); ++i) {
        char c = json[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '[') ++depth;
        else if (c == ']') {
            --depth;
            if (depth == 0) return json.substr(blockStart, i - blockStart + 1);
        }
    }
    return "";
}

std::vector<std::string> ExtractObjectBlocks(const std::string& arrayBlock) {
    std::vector<std::string> blocks;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    std::size_t objectStart = std::string::npos;

    for (std::size_t i = 0; i < arrayBlock.size(); ++i) {
        char c = arrayBlock[i];
        if (inString) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') {
            inString = true;
        } else if (c == '{') {
            if (depth == 0) objectStart = i;
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0 && objectStart != std::string::npos) {
                blocks.push_back(arrayBlock.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        }
    }
    return blocks;
}

std::string UnescapeJsonString(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '\\' && i + 1 < value.size()) {
            char escaped = value[++i];
            switch (escaped) {
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                default: out.push_back(escaped); break;
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

std::string ExtractString(const std::string& json, const std::string& key, const std::string& fallbackValue) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return fallbackValue;

    std::size_t colon = json.find(':', keyPos);
    if (colon == std::string::npos) return fallbackValue;

    std::size_t valueStart = json.find('"', colon + 1);
    if (valueStart == std::string::npos) return fallbackValue;
    ++valueStart;

    std::string value;
    bool escaped = false;
    for (std::size_t i = valueStart; i < json.size(); ++i) {
        char c = json[i];
        if (escaped) {
            value.push_back('\\');
            value.push_back(c);
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"') return UnescapeJsonString(value);
        value.push_back(c);
    }
    return fallbackValue;
}

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

    std::vector<std::string> blocks = ExtractObjectBlocks(ExtractArrayBlock(json, "levels"));
    if (blocks.empty()) return config;

    config.levels.clear();
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        LevelConfigEntry entry;
        entry.name = ExtractString(blocks[i], "name", "");
        entry.path = ExtractString(blocks[i], "path", "");
        entry.configPath = ExtractString(blocks[i], "config", "");
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
