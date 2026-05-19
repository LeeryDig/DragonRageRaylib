#ifndef DEBUG_DEBUG_ICONS_HPP
#define DEBUG_DEBUG_ICONS_HPP

#include "raylib.h"

struct DebugIcons {
    Font materialSymbols;
    bool loaded;

    DebugIcons() : materialSymbols(), loaded(false) {}
};

bool LoadDebugIcons(DebugIcons& icons);
void UnloadDebugIcons(DebugIcons& icons);
void DrawDebugIcon2D(const DebugIcons& icons, const char* iconName, Vector2 pos, float size, Color color);
void DrawDebugIconBillboard(const DebugIcons& icons, const Camera& camera, const char* iconName, Vector3 worldPos, float size, Color color);

#endif
