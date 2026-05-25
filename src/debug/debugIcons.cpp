#include "debug/debugIcons.hpp"

#include <cstring>

#include "raymath.h"
#include "utils.hpp"

namespace {

const char* MaterialGlyph(const char* iconName) {
    if (std::strcmp(iconName, "delete") == 0) return "\xEE\xA1\xB2";      // e872
    if (std::strcmp(iconName, "add") == 0) return "\xEE\x85\x85";         // e145
    if (std::strcmp(iconName, "lightbulb") == 0) return "\xEE\x83\xB0";   // e0f0
    if (std::strcmp(iconName, "wb_sunny") == 0) return "\xEE\x90\xB0";    // e430
    if (std::strcmp(iconName, "save") == 0) return "\xEE\x85\xA1";        // e161
    if (std::strcmp(iconName, "person") == 0) return "\xEE\x9F\xBD";      // e7fd
    if (std::strcmp(iconName, "map") == 0) return "\xEE\x95\x9B";         // e55b
    if (std::strcmp(iconName, "flashlight_on") == 0) return "SP";
    if (std::strcmp(iconName, "smoking_rooms") == 0) return "\xEE\x86\xAA";  // e1aa
    return iconName;
}

const char* FallbackLabel(const char* iconName) {
    if (std::strcmp(iconName, "delete") == 0) return "DEL";
    if (std::strcmp(iconName, "add") == 0) return "+";
    if (std::strcmp(iconName, "wb_sunny") == 0) return "SUN";
    if (std::strcmp(iconName, "lightbulb") == 0) return "PT";
    if (std::strcmp(iconName, "flashlight_on") == 0) return "SP";
    if (std::strcmp(iconName, "person") == 0) return "CHR";
    if (std::strcmp(iconName, "map") == 0) return "MAP";
    if (std::strcmp(iconName, "save") == 0) return "SAVE";
    if (std::strcmp(iconName, "smoking_rooms") == 0) return "SMK";
    return iconName;
}

}  // namespace

bool LoadDebugIcons(DebugIcons& icons) {
    const char* path = "resources/icons/Material_Symbols_Rounded/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf";
    int codepoints[] = {0xe872, 0xe145, 0xe0f0, 0xe430, 0xe161, 0xe7fd, 0xe55b, 0xe1aa};
    icons.materialSymbols = LoadFontEx(Utils::ResolveProjectPath(path).c_str(), 64, codepoints, sizeof(codepoints) / sizeof(codepoints[0]));
    icons.loaded = icons.materialSymbols.texture.id != 0;
    if (!icons.loaded) {
        TraceLog(LOG_WARNING, "Debug icons: failed to load Material Symbols Rounded");
    }
    return icons.loaded;
}

void UnloadDebugIcons(DebugIcons& icons) {
    if (icons.loaded) {
        UnloadFont(icons.materialSymbols);
    }
    icons = DebugIcons();
}

void DrawDebugIcon2D(const DebugIcons& icons, const char* iconName, Vector2 pos, float size, Color color) {
    if (icons.loaded) {
        const char* glyph = MaterialGlyph(iconName);
        DrawTextEx(icons.materialSymbols, glyph, pos, size, 0.0f, color);
        return;
    }
    DrawText(FallbackLabel(iconName), static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(size * 0.55f), color);
}

void DrawDebugIconBillboard(const DebugIcons& icons, const Camera& camera, const char* iconName, Vector3 worldPos, float size, Color color) {
    Vector2 pos = GetWorldToScreen(worldPos, camera);
    DrawDebugIcon2D(icons, iconName, Vector2{pos.x - size * 0.5f, pos.y - size * 0.5f}, size, color);
}
