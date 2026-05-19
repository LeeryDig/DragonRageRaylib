#include "levelRuntimeConfig.hpp"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

#include "raylib.h"
#include "utils.hpp"

namespace {

struct JsonValue {
    enum Type { Null, Bool, Number, String, Array, Object } type;
    bool boolValue;
    double numberValue;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    JsonValue() : type(Null), boolValue(false), numberValue(0.0) {}
};

class JsonParser {
  public:
    explicit JsonParser(const std::string& textIn) : text(textIn), pos(0) {}
    JsonValue Parse() { SkipWhitespace(); return ParseValue(); }

  private:
    const std::string& text;
    std::size_t pos;

    void SkipWhitespace() {
        while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\n' || text[pos] == '\r' || text[pos] == '\t')) ++pos;
    }

    bool Match(const char* token) {
        std::size_t len = std::char_traits<char>::length(token);
        if (text.compare(pos, len, token) == 0) { pos += len; return true; }
        return false;
    }

    JsonValue ParseValue() {
        SkipWhitespace();
        if (pos >= text.size()) return JsonValue();
        char c = text[pos];
        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == '"') return ParseString();
        if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber();
        JsonValue value;
        if (Match("true")) { value.type = JsonValue::Bool; value.boolValue = true; return value; }
        if (Match("false")) { value.type = JsonValue::Bool; value.boolValue = false; return value; }
        Match("null");
        return value;
    }

    JsonValue ParseObject() {
        JsonValue value; value.type = JsonValue::Object; ++pos; SkipWhitespace();
        if (pos < text.size() && text[pos] == '}') { ++pos; return value; }
        while (pos < text.size()) {
            SkipWhitespace();
            JsonValue key = ParseString(); SkipWhitespace();
            if (pos < text.size() && text[pos] == ':') ++pos;
            value.objectValue[key.stringValue] = ParseValue(); SkipWhitespace();
            if (pos < text.size() && text[pos] == ',') { ++pos; continue; }
            if (pos < text.size() && text[pos] == '}') { ++pos; break; }
        }
        return value;
    }

    JsonValue ParseArray() {
        JsonValue value; value.type = JsonValue::Array; ++pos; SkipWhitespace();
        if (pos < text.size() && text[pos] == ']') { ++pos; return value; }
        while (pos < text.size()) {
            value.arrayValue.push_back(ParseValue()); SkipWhitespace();
            if (pos < text.size() && text[pos] == ',') { ++pos; continue; }
            if (pos < text.size() && text[pos] == ']') { ++pos; break; }
        }
        return value;
    }

    JsonValue ParseString() {
        JsonValue value; value.type = JsonValue::String;
        if (pos >= text.size() || text[pos] != '"') return value;
        ++pos;
        while (pos < text.size()) {
            char c = text[pos++];
            if (c == '"') break;
            if (c == '\\' && pos < text.size()) {
                char escaped = text[pos++];
                switch (escaped) {
                    case '"': value.stringValue.push_back('"'); break;
                    case '\\': value.stringValue.push_back('\\'); break;
                    case '/': value.stringValue.push_back('/'); break;
                    case 'b': value.stringValue.push_back('\b'); break;
                    case 'f': value.stringValue.push_back('\f'); break;
                    case 'n': value.stringValue.push_back('\n'); break;
                    case 'r': value.stringValue.push_back('\r'); break;
                    case 't': value.stringValue.push_back('\t'); break;
                    default: value.stringValue.push_back(escaped); break;
                }
            } else {
                value.stringValue.push_back(c);
            }
        }
        return value;
    }

    JsonValue ParseNumber() {
        JsonValue value; value.type = JsonValue::Number;
        const char* start = text.c_str() + pos;
        char* end = nullptr;
        value.numberValue = std::strtod(start, &end);
        pos += static_cast<std::size_t>(end - start);
        return value;
    }
};

const JsonValue* GetMember(const JsonValue& value, const char* name) {
    if (value.type != JsonValue::Object) return nullptr;
    std::map<std::string, JsonValue>::const_iterator it = value.objectValue.find(name);
    return it == value.objectValue.end() ? nullptr : &it->second;
}

std::string StringMember(const JsonValue& value, const char* name, const std::string& fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::String ? member->stringValue : fallback;
}

bool BoolMember(const JsonValue& value, const char* name, bool fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::Bool ? member->boolValue : fallback;
}

float NumberMember(const JsonValue& value, const char* name, float fallback) {
    const JsonValue* member = GetMember(value, name);
    return member && member->type == JsonValue::Number ? static_cast<float>(member->numberValue) : fallback;
}

int ColorChannelAt(const JsonValue* arrayValue, std::size_t index, unsigned char fallback) {
    if (!arrayValue || arrayValue->type != JsonValue::Array || index >= arrayValue->arrayValue.size()) return fallback;
    const JsonValue& value = arrayValue->arrayValue[index];
    if (value.type != JsonValue::Number) return fallback;
    int channel = static_cast<int>(value.numberValue);
    if (channel < 0) return 0;
    if (channel > 255) return 255;
    return channel;
}

Color ColorMember(const JsonValue& value, const char* name, Color fallback) {
    const JsonValue* member = GetMember(value, name);
    return Color{
        static_cast<unsigned char>(ColorChannelAt(member, 0, fallback.r)),
        static_cast<unsigned char>(ColorChannelAt(member, 1, fallback.g)),
        static_cast<unsigned char>(ColorChannelAt(member, 2, fallback.b)),
        255
    };
}

FogMode FogModeFromString(const std::string& mode) {
    if (mode == "linear") return FogMode::Linear;
    return FogMode::Linear;
}

const char* FogModeToString(FogMode mode) {
    switch (mode) {
        case FogMode::Linear: return "linear";
    }
    return "linear";
}

float NumberAt(const JsonValue* arrayValue, std::size_t index, float fallback) {
    if (!arrayValue || arrayValue->type != JsonValue::Array || index >= arrayValue->arrayValue.size()) return fallback;
    const JsonValue& value = arrayValue->arrayValue[index];
    return value.type == JsonValue::Number ? static_cast<float>(value.numberValue) : fallback;
}

Vector3 Vector3Member(const JsonValue& value, const char* name, Vector3 fallback) {
    const JsonValue* member = GetMember(value, name);
    return Vector3{NumberAt(member, 0, fallback.x), NumberAt(member, 1, fallback.y), NumberAt(member, 2, fallback.z)};
}

}  // namespace

LevelRuntimeConfig LoadLevelRuntimeConfig(const std::string& configPath) {
    LevelRuntimeConfig config;
    if (configPath.empty()) return config;

    char* raw = LoadFileText(Utils::ResolveProjectPath(configPath).c_str());
    if (raw == nullptr) {
        TraceLog(LOG_WARNING, "Level config: failed to load %s", configPath.c_str());
        return config;
    }

    std::string json = raw;
    UnloadFileText(raw);
    JsonValue root = JsonParser(json).Parse();
    if (root.type != JsonValue::Object) return config;

    config.skyboxPath = StringMember(root, "skybox", "");

    const JsonValue* fog = GetMember(root, "fog");
    if (fog && fog->type == JsonValue::Object) {
        config.fog.enabled = BoolMember(*fog, "enabled", config.fog.enabled);
        config.fog.color = ColorMember(*fog, "color", config.fog.color);
        config.fog.start = NumberMember(*fog, "start", config.fog.start);
        config.fog.end = NumberMember(*fog, "end", config.fog.end);
        config.fog.density = NumberMember(*fog, "density", config.fog.density);
        config.fog.mode = FogModeFromString(StringMember(*fog, "mode", FogModeToString(config.fog.mode)));
    }

    const JsonValue* characters = GetMember(root, "characters");
    if (characters && characters->type == JsonValue::Array) {
        for (std::size_t i = 0; i < characters->arrayValue.size(); ++i) {
            const JsonValue& item = characters->arrayValue[i];
            if (item.type != JsonValue::Object) continue;

            CharacterSpawnConfig character;
            character.configPath = StringMember(item, "config", "");
            character.position = Vector3Member(item, "position", Vector3{0.0f, 0.0f, 0.0f});
            character.rotationDegrees = Vector3Member(item, "rotation", Vector3{0.0f, 0.0f, 0.0f});
            if (!character.configPath.empty()) config.characters.push_back(character);
        }
    }

    TraceLog(LOG_INFO,
        "Level config: loaded %s skybox=%s fog=%s characters=%zu",
        configPath.c_str(),
        config.skyboxPath.empty() ? "(none)" : config.skyboxPath.c_str(),
        config.fog.enabled ? "on" : "off",
        config.characters.size());
    return config;
}

bool SaveLevelRuntimeConfig(const std::string& configPath, const LevelRuntimeConfig& config) {
    if (configPath.empty()) return false;

    std::ofstream file(Utils::ResolveWritableProjectPath(configPath).c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "Level config: failed to save %s", configPath.c_str());
        return false;
    }

    file << std::fixed << std::setprecision(3);
    file << "{\n";
    if (!config.skyboxPath.empty()) {
        file << "  \"skybox\": \"" << config.skyboxPath << "\",\n";
    }
    file << "  \"fog\": {\n";
    file << "    \"enabled\": " << (config.fog.enabled ? "true" : "false") << ",\n";
    file << "    \"color\": [" << static_cast<int>(config.fog.color.r) << ", " << static_cast<int>(config.fog.color.g) << ", " << static_cast<int>(config.fog.color.b) << "],\n";
    file << "    \"start\": " << config.fog.start << ",\n";
    file << "    \"end\": " << config.fog.end << ",\n";
    file << "    \"density\": " << config.fog.density << ",\n";
    file << "    \"mode\": \"" << FogModeToString(config.fog.mode) << "\"\n";
    file << "  },\n";
    file << "  \"characters\": [\n";
    for (std::size_t i = 0; i < config.characters.size(); ++i) {
        const CharacterSpawnConfig& character = config.characters[i];
        file << "    {\n";
        file << "      \"config\": \"" << character.configPath << "\",\n";
        file << "      \"position\": [" << character.position.x << ", " << character.position.y << ", " << character.position.z << "],\n";
        file << "      \"rotation\": [" << character.rotationDegrees.x << ", " << character.rotationDegrees.y << ", " << character.rotationDegrees.z << "]\n";
        file << "    }" << (i + 1 < config.characters.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    TraceLog(LOG_INFO, "Level config: saved %s", configPath.c_str());
    return true;
}
