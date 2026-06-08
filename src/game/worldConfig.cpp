#include "worldConfig.hpp"

#include <string>

#include <raylib.h>

#include "assets/json.hpp"

namespace {

using assets::BoolMember;
using assets::IntMember;
using assets::JsonParser;
using assets::JsonValue;
using assets::StringMember;

}  // namespace

WorldConfig DefaultWorldConfig() {
    return WorldConfig{1920, 1080, "Dragon Rage", 60, true};
}

WorldConfig LoadWorldConfig(const std::string& filePath, const WorldConfig& fallback) {
    WorldConfig config = fallback;

    char* raw = LoadFileText(filePath.c_str());
    if (raw == nullptr) return config;
    std::string json = raw;
    UnloadFileText(raw);

    JsonParser parser(json);
    JsonValue root = parser.Parse();
    if (parser.HadError()) {
        TraceLog(LOG_WARNING, "WorldConfig: JSON parse warning in %s: %s", filePath.c_str(), parser.Error().c_str());
    }
    if (root.type != JsonValue::Object) {
        TraceLog(LOG_WARNING, "WorldConfig: root is not object in %s", filePath.c_str());
        return config;
    }

    int w = IntMember(root, "window_width", fallback.windowWidth);
    if (w > 0) config.windowWidth = w;

    int h = IntMember(root, "window_height", fallback.windowHeight);
    if (h > 0) config.windowHeight = h;

    std::string title = StringMember(root, "window_title", "");
    if (!title.empty()) config.windowTitle = title;

    int fps = IntMember(root, "target_fps", fallback.targetFps);
    if (fps > 0) config.targetFps = fps;

    config.fullscreen = BoolMember(root, "fullscreen", fallback.fullscreen);

    return config;
}
