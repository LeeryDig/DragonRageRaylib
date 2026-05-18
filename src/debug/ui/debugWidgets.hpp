#ifndef DEBUG_UI_DEBUG_WIDGETS_HPP
#define DEBUG_UI_DEBUG_WIDGETS_HPP

#include <string>

#include "raylib.h"

#include "game/gameWorld.hpp"

namespace debug_ui {

bool DebugMenuItem(Rectangle rect, const char* text, bool enabled = true, bool checked = false);
bool DebugButton(Rectangle rect, const char* text);
bool DebugTextInput(Rectangle rect, const char* label, std::string& text, int fieldId, DebugUiState& ui);
bool DebugFloatSlider(Rectangle rect, const char* label, float& value, float minValue, float maxValue);
void DrawVector3Value(Vector2 pos, const char* label, const Vector3& value);

bool BeginDebugPanel(
    DebugUiState& ui,
    int panelId,
    Vector2& position,
    bool& pinned,
    const char* title,
    float width,
    float height);

}  // namespace debug_ui

#endif
