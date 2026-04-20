#include "ui.hpp"

namespace UI {

namespace {

Color GetButtonColor(bool hovered, bool enabled) {
    if (!enabled) {
        return Color{120, 120, 120, 255};
    }

    return hovered ? Color{90, 90, 140, 255} : Color{55, 55, 95, 255};
}

void DrawCenteredText(const std::string& text, Rectangle bounds, int fontSize, Color color) {
    int textWidth = MeasureText(text.c_str(), fontSize);
    int textX = static_cast<int>(bounds.x + (bounds.width - textWidth) * 0.5f);
    int textY = static_cast<int>(bounds.y + (bounds.height - fontSize) * 0.5f);
    DrawText(text.c_str(), textX, textY, fontSize, color);
}

}  // namespace

bool DrawButton(Rectangle bounds, const std::string& label, bool enabled) {
    Vector2 mousePosition = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mousePosition, bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    DrawRectangleRounded(bounds, 0.18f, 10, GetButtonColor(hovered, enabled));
    DrawRectangleRoundedLinesEx(bounds, 0.18f, 10, 2.0f, BLACK);
    DrawCenteredText(label, bounds, 24, enabled ? RAYWHITE : LIGHTGRAY);

    return clicked;
}

void OpenConfirmationDialog(
    ConfirmationDialog& dialog,
    const std::string& title,
    const std::string& message,
    const std::string& confirmLabel,
    const std::string& cancelLabel) {
    dialog.isOpen = true;
    dialog.ignoreClickUntilRelease = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    dialog.title = title;
    dialog.message = message;
    dialog.confirmLabel = confirmLabel;
    dialog.cancelLabel = cancelLabel;
}

void CloseConfirmationDialog(ConfirmationDialog& dialog) {
    dialog.isOpen = false;
    dialog.ignoreClickUntilRelease = false;
}

ModalResult DrawConfirmationDialog(
    ConfirmationDialog& dialog,
    int screenWidth,
    int screenHeight) {
    if (!dialog.isOpen) {
        return ModalResult::NONE;
    }

    Rectangle overlay = {0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight)};
    Rectangle panel = {
        static_cast<float>(screenWidth) * 0.5f - 220.0f,
        static_cast<float>(screenHeight) * 0.5f - 110.0f,
        440.0f,
        220.0f
    };

    Rectangle confirmButton = {
        panel.x + 40.0f,
        panel.y + panel.height - 70.0f,
        150.0f,
        44.0f
    };
    Rectangle cancelButton = {
        panel.x + panel.width - 190.0f,
        panel.y + panel.height - 70.0f,
        150.0f,
        44.0f
    };

    DrawRectangleRec(overlay, Color{0, 0, 0, 160});
    DrawRectangleRounded(panel, 0.08f, 10, RAYWHITE);
    DrawRectangleRoundedLinesEx(panel, 0.08f, 10, 2.0f, DARKGRAY);

    DrawText(dialog.title.c_str(), static_cast<int>(panel.x) + 24, static_cast<int>(panel.y) + 24, 30, BLACK);
    DrawText(dialog.message.c_str(), static_cast<int>(panel.x) + 24, static_cast<int>(panel.y) + 82, 24, DARKGRAY);

    if (dialog.ignoreClickUntilRelease && !IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        dialog.ignoreClickUntilRelease = false;
    }

    bool buttonsEnabled = !dialog.ignoreClickUntilRelease;
    if (DrawButton(confirmButton, dialog.confirmLabel, buttonsEnabled)) {
        CloseConfirmationDialog(dialog);
        return ModalResult::CONFIRMED;
    }

    if (DrawButton(cancelButton, dialog.cancelLabel, buttonsEnabled)) {
        CloseConfirmationDialog(dialog);
        return ModalResult::CANCELLED;
    }

    return ModalResult::NONE;
}

}  // namespace UI
