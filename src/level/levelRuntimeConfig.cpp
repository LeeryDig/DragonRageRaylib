#include "levelRuntimeConfig.hpp"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

#include "raylib.h"
#include "raymath.h"
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

LightType LightTypeFromString(const std::string& type) {
    if (type == "point") return LightType::Point;
    if (type == "spot") return LightType::Spot;
    return LightType::Directional;
}

const char* LightTypeToString(LightType type) {
    switch (type) {
        case LightType::Directional: return "directional";
        case LightType::Point: return "point";
        case LightType::Spot: return "spot";
    }
    return "directional";
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

Quaternion QuaternionFromEulerDegrees(Vector3 degrees) {
    return QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
}

Vector3 EulerDegreesFromQuaternion(Quaternion rotation) {
    Vector3 radians = QuaternionToEuler(rotation);
    return Vector3{radians.x * RAD2DEG, radians.y * RAD2DEG, radians.z * RAD2DEG};
}

Vector3 DirectionToEulerDegrees(Vector3 direction) {
    direction = Vector3Normalize(direction);
    float yaw = atan2f(direction.x, -direction.z);
    float pitch = asinf(Clamp(direction.y, -1.0f, 1.0f));
    return Vector3{pitch * RAD2DEG, yaw * RAD2DEG, 0.0f};
}

void EnsureDefaultSun(LightingConfig& lighting) {
    if (!lighting.lights.empty()) return;
    LevelLightConfig sun;
    sun.id = "sun";
    sun.type = LightType::Directional;
    sun.rotation = QuaternionFromEulerDegrees(DirectionToEulerDegrees(Vector3{-0.4f, -1.0f, -0.3f}));
    lighting.lights.push_back(sun);
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

    const JsonValue* lighting = GetMember(root, "lighting");
    if (lighting && lighting->type == JsonValue::Object) {
        config.lighting.ambient = ColorMember(*lighting, "ambient", config.lighting.ambient);
        const JsonValue* lights = GetMember(*lighting, "lights");
        if (lights && lights->type == JsonValue::Array) {
            config.lighting.lights.clear();
            for (std::size_t i = 0; i < lights->arrayValue.size(); ++i) {
                const JsonValue& item = lights->arrayValue[i];
                if (item.type != JsonValue::Object) continue;
                LevelLightConfig light;
                light.id = StringMember(item, "id", i == 0 ? "sun" : "light");
                light.type = LightTypeFromString(StringMember(item, "type", LightTypeToString(light.type)));
                light.enabled = BoolMember(item, "enabled", light.enabled);
                light.position = Vector3Member(item, "position", light.position);
                light.rotation = QuaternionFromEulerDegrees(Vector3Member(item, "rotation", EulerDegreesFromQuaternion(light.rotation)));
                light.color = ColorMember(item, "color", light.color);
                light.intensity = NumberMember(item, "intensity", light.intensity);
                light.castShadows = BoolMember(item, "castShadows", light.castShadows);
                config.lighting.lights.push_back(light);
            }
        } else {
            const JsonValue* directional = GetMember(*lighting, "directional");
            if (directional && directional->type == JsonValue::Object) {
                LevelLightConfig light;
                light.id = "sun";
                light.type = LightType::Directional;
                light.enabled = BoolMember(*directional, "enabled", light.enabled);
                light.position = Vector3Member(*directional, "position", light.position);
                Vector3 fallbackDirection = Vector3Member(*directional, "target", Vector3{-0.4f, -1.0f, -0.3f});
                light.rotation = QuaternionFromEulerDegrees(DirectionToEulerDegrees(fallbackDirection));
                light.color = ColorMember(*directional, "color", light.color);
                light.intensity = NumberMember(*directional, "intensity", light.intensity);
                light.castShadows = BoolMember(*directional, "castShadows", light.castShadows);
                config.lighting.lights.clear();
                config.lighting.lights.push_back(light);
            }
        }
    }
    EnsureDefaultSun(config.lighting);

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
        "Level config: loaded %s skybox=%s fog=%s lights=%zu characters=%zu",
        configPath.c_str(),
        config.skyboxPath.empty() ? "(none)" : config.skyboxPath.c_str(),
        config.fog.enabled ? "on" : "off",
        config.lighting.lights.size(),
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
    file << "  \"lighting\": {\n";
    file << "    \"ambient\": [" << static_cast<int>(config.lighting.ambient.r) << ", " << static_cast<int>(config.lighting.ambient.g) << ", " << static_cast<int>(config.lighting.ambient.b) << "],\n";
    file << "    \"lights\": [\n";
    for (std::size_t i = 0; i < config.lighting.lights.size(); ++i) {
        const LevelLightConfig& light = config.lighting.lights[i];
        Vector3 rotationDegrees = EulerDegreesFromQuaternion(light.rotation);
        file << "      {\n";
        file << "        \"id\": \"" << light.id << "\",\n";
        file << "        \"type\": \"" << LightTypeToString(light.type) << "\",\n";
        file << "        \"enabled\": " << (light.enabled ? "true" : "false") << ",\n";
        file << "        \"position\": [" << light.position.x << ", " << light.position.y << ", " << light.position.z << "],\n";
        file << "        \"rotation\": [" << rotationDegrees.x << ", " << rotationDegrees.y << ", " << rotationDegrees.z << "],\n";
        file << "        \"color\": [" << static_cast<int>(light.color.r) << ", " << static_cast<int>(light.color.g) << ", " << static_cast<int>(light.color.b) << "],\n";
        file << "        \"intensity\": " << light.intensity << ",\n";
        file << "        \"castShadows\": " << (light.castShadows ? "true" : "false") << "\n";
        file << "      }" << (i + 1 < config.lighting.lights.size() ? "," : "") << "\n";
    }
    file << "    ]\n";
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
