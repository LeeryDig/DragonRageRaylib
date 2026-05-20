#include "worldConfig.hpp"

#include <string>

#include <raylib.h>

namespace {

int ExtractInt(const std::string& json, const std::string& key, int fallback) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return fallback;
    std::size_t colon = json.find(':', keyPos);
    if (colon == std::string::npos) return fallback;
    char* end = nullptr;
    long value = std::strtol(json.c_str() + colon + 1, &end, 10);
    return (end == json.c_str() + colon + 1) ? fallback : static_cast<int>(value);
}

std::string ExtractString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    std::size_t colon = json.find(':', keyPos);
    if (colon == std::string::npos) return "";
    std::size_t openQuote = json.find('"', colon + 1);
    if (openQuote == std::string::npos) return "";
    std::size_t closeQuote = json.find('"', openQuote + 1);
    if (closeQuote == std::string::npos) return "";
    return json.substr(openQuote + 1, closeQuote - openQuote - 1);
}

}  // namespace

WorldConfig DefaultWorldConfig() {
    return WorldConfig{1280, 720, "Dragon Rage", 60};
}

WorldConfig LoadWorldConfig(const std::string& filePath, const WorldConfig& fallback) {
    WorldConfig config = fallback;

    char* raw = LoadFileText(filePath.c_str());
    if (raw == nullptr) return config;
    std::string json = raw;
    UnloadFileText(raw);

    int w = ExtractInt(json, "window_width", fallback.windowWidth);
    if (w > 0) config.windowWidth = w;

    int h = ExtractInt(json, "window_height", fallback.windowHeight);
    if (h > 0) config.windowHeight = h;

    std::string title = ExtractString(json, "window_title");
    if (!title.empty()) config.windowTitle = title;

    int fps = ExtractInt(json, "target_fps", fallback.targetFps);
    if (fps > 0) config.targetFps = fps;

    return config;
}
