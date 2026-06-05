#include "uiText.hpp"
#include "debug/ui/debugWidgets.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "raymath.h"

namespace debug_ui {

bool DebugMenuItem(Rectangle rect, const char* text, bool enabled, bool checked) {
    Vector2 mouse = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovered ? Color{70, 70, 78, 255} : Color{42, 42, 48, 245});
    DrawRectangleLinesEx(rect, 1.0f, Color{78, 78, 86, 255});
    if (checked) {
        DrawUiText("✓", static_cast<int>(rect.x + 8), static_cast<int>(rect.y + 5), 18, RAYWHITE);
    }
    DrawUiText(text, static_cast<int>(rect.x + 28), static_cast<int>(rect.y + 6), 18, enabled ? RAYWHITE : GRAY);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool DebugButton(Rectangle rect, const char* text) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovered ? Color{78, 78, 88, 255} : Color{56, 56, 64, 255});
    DrawRectangleLinesEx(rect, 1.0f, Color{95, 95, 105, 255});
    int textWidth = MeasureUiText(text, 16);
    DrawUiText(text, static_cast<int>(rect.x + rect.width * 0.5f - textWidth * 0.5f), static_cast<int>(rect.y + 6), 16, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool DebugTextInput(Rectangle rect, const char* label, std::string& text, int fieldId, DebugUiState& ui) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.activeTextField = fieldId;
    }

    bool active = ui.activeTextField == fieldId;
    bool changed = false;
    DrawUiText(label, static_cast<int>(rect.x), static_cast<int>(rect.y + 5), 16, RAYWHITE);

    Rectangle inputRect = Rectangle{rect.x + 24.0f, rect.y, rect.width - 24.0f, rect.height};
    DrawRectangleRec(inputRect, active ? Color{46, 56, 72, 255} : Color{36, 36, 42, 255});
    DrawRectangleLinesEx(inputRect, 1.0f, active ? Color{120, 150, 220, 255} : Color{85, 85, 95, 255});
    DrawUiText(text.c_str(), static_cast<int>(inputRect.x + 6.0f), static_cast<int>(inputRect.y + 5.0f), 16, RAYWHITE);

    if (active) {
        int key = GetCharPressed();
        while (key > 0) {
            char c = static_cast<char>(key);
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
                if (text.size() < 20) {
                    text.push_back(c);
                    changed = true;
                }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) {
            text.pop_back();
            changed = true;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            ui.activeTextField = 0;
        }
    }
    return changed;
}

bool DebugFloatSlider(Rectangle rect, const char* label, float& value, float minValue, float maxValue) {
    Vector2 mouse = GetMousePosition();
    Rectangle bar = Rectangle{rect.x + 150.0f, rect.y + 8.0f, rect.width - 230.0f, 8.0f};
    bool changed = false;
    if (CheckCollisionPointRec(mouse, Rectangle{bar.x, rect.y, bar.width, rect.height}) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        float t = Clamp((mouse.x - bar.x) / bar.width, 0.0f, 1.0f);
        value = minValue + (maxValue - minValue) * t;
        changed = true;
    }

    float normalized = Clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
    DrawUiText(label, static_cast<int>(rect.x), static_cast<int>(rect.y + 2), 16, RAYWHITE);
    DrawRectangleRec(bar, Color{55, 55, 62, 255});
    DrawRectangleRec(Rectangle{bar.x, bar.y, bar.width * normalized, bar.height}, Color{90, 130, 210, 255});
    DrawCircle(static_cast<int>(bar.x + bar.width * normalized), static_cast<int>(bar.y + bar.height * 0.5f), 6.0f, RAYWHITE);
    DrawUiText(TextFormat("%.3f", value), static_cast<int>(rect.x + rect.width - 70.0f), static_cast<int>(rect.y + 2), 16, LIGHTGRAY);
    return changed;
}

bool DebugFloatInputRow(Rectangle rect, const char* label, float& value, float minValue, float maxValue, int fieldId, DebugUiState& ui) {
    constexpr float INPUT_W = 88.0f;
    constexpr float GAP     = 4.0f;

    Rectangle sliderRect = {rect.x, rect.y, rect.width - INPUT_W - GAP, rect.height};
    Rectangle inputRect  = {rect.x + rect.width - INPUT_W, rect.y + 4.0f, INPUT_W, rect.height - 8.0f};

    // Slider (reuse existing but skip its value text — covered by input box)
    Rectangle bar = {sliderRect.x + 150.0f, sliderRect.y + 8.0f, sliderRect.width - 155.0f, 8.0f};
    bool sliderChanged = false;
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, Rectangle{bar.x, rect.y, bar.width, rect.height}) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        float t = Clamp((mouse.x - bar.x) / bar.width, 0.0f, 1.0f);
        value = minValue + (maxValue - minValue) * t;
        sliderChanged = true;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.4f", value);
        if (ui.activeTextField != fieldId) ui.activeTextFieldBuffer = buf;
    }
    float normalized = Clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
    DrawUiText(label, static_cast<int>(sliderRect.x), static_cast<int>(sliderRect.y + 2), 16, RAYWHITE);
    DrawRectangleRec(bar, Color{55, 55, 62, 255});
    DrawRectangleRec(Rectangle{bar.x, bar.y, bar.width * normalized, bar.height}, Color{90, 130, 210, 255});
    DrawCircle(static_cast<int>(bar.x + bar.width * normalized), static_cast<int>(bar.y + bar.height * 0.5f), 6.0f, RAYWHITE);

    // Input box
    bool hovered = CheckCollisionPointRec(mouse, inputRect);
    bool active  = ui.activeTextField == fieldId;
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.activeTextField = fieldId;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.4f", value);
        ui.activeTextFieldBuffer = buf;
    }
    DrawRectangleRec(inputRect, active ? Color{46, 56, 72, 255} : Color{36, 36, 42, 255});
    DrawRectangleLinesEx(inputRect, 1.0f, active ? Color{120, 150, 220, 255} : Color{85, 85, 95, 255});
    const char* displayStr = active ? ui.activeTextFieldBuffer.c_str() : TextFormat("%.4f", value);
    DrawUiText(displayStr, static_cast<int>(inputRect.x + 4.0f), static_cast<int>(inputRect.y + 3.0f), 14, RAYWHITE);

    bool inputChanged = false;
    if (active) {
        int key = GetCharPressed();
        while (key > 0) {
            char c = static_cast<char>(key);
            if ((c >= '0' && c <= '9') || c == '-' || c == '.') {
                if (ui.activeTextFieldBuffer.size() < 16) ui.activeTextFieldBuffer.push_back(c);
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !ui.activeTextFieldBuffer.empty()) {
            ui.activeTextFieldBuffer.pop_back();
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            try { value = std::stof(ui.activeTextFieldBuffer); } catch (...) {}
            value = Clamp(value, minValue, maxValue);
            ui.activeTextField = 0;
            inputChanged = true;
        }
    }

    return sliderChanged || inputChanged;
}

void DrawVector3Value(Vector2 pos, const char* label, const Vector3& value) {
    DrawUiText(TextFormat("%s: %.2f %.2f %.2f", label, value.x, value.y, value.z), static_cast<int>(pos.x), static_cast<int>(pos.y), 16, LIGHTGRAY);
}

bool BeginDebugPanel(
    DebugUiState& ui,
    int panelId,
    Vector2& position,
    bool& pinned,
    const char* title,
    float width,
    float height) {
    Vector2 mouse = GetMousePosition();
    Rectangle panelRect = Rectangle{position.x, position.y, width, height};
    Rectangle titleRect = Rectangle{position.x, position.y, width, 28.0f};
    Rectangle pinRect = Rectangle{position.x + width - 30.0f, position.y + 4.0f, 22.0f, 20.0f};

    if (ui.enabled && CheckCollisionPointRec(mouse, pinRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        pinned = !pinned;
    }

    if (ui.enabled && CheckCollisionPointRec(mouse, titleRect) && !CheckCollisionPointRec(mouse, pinRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.draggingPanel = panelId;
        ui.dragOffset = Vector2Subtract(mouse, position);
    }
    if (ui.draggingPanel == panelId) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            position = Vector2Subtract(mouse, ui.dragOffset);
            position.x = Clamp(position.x, 0.0f, static_cast<float>(GetScreenWidth()) - width);
            position.y = Clamp(position.y, 0.0f, static_cast<float>(GetScreenHeight()) - 28.0f);
        } else {
            ui.draggingPanel = -1;
        }
    }

    DrawRectangleRec(panelRect, Color{28, 28, 32, 220});
    DrawRectangleRec(titleRect, Color{42, 42, 48, 245});
    DrawRectangleLinesEx(panelRect, 1.0f, Color{80, 80, 88, 255});
    DrawUiText(title, static_cast<int>(position.x + 12.0f), static_cast<int>(position.y + 6.0f), 18, RAYWHITE);
    DrawRectangleRec(pinRect, pinned ? Color{80, 120, 80, 255} : Color{65, 65, 72, 255});
    DrawRectangleLinesEx(pinRect, 1.0f, Color{95, 95, 105, 255});
    DrawUiText("P", static_cast<int>(pinRect.x + 6.0f), static_cast<int>(pinRect.y + 2.0f), 16, RAYWHITE);
    return true;
}

}  // namespace debug_ui
