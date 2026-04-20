#ifndef UI_HPP
#define UI_HPP

#include <string>

#include "raylib.h"

namespace UI {

enum class ModalResult {
    NONE,
    CONFIRMED,
    CANCELLED
};

struct ConfirmationDialog {
    bool isOpen;
    bool ignoreClickUntilRelease;
    std::string title;
    std::string message;
    std::string confirmLabel;
    std::string cancelLabel;
};

bool DrawButton(Rectangle bounds, const std::string& label, bool enabled = true);

void OpenConfirmationDialog(
    ConfirmationDialog& dialog,
    const std::string& title,
    const std::string& message,
    const std::string& confirmLabel = "Sim",
    const std::string& cancelLabel = "Nao");

void CloseConfirmationDialog(ConfirmationDialog& dialog);

ModalResult DrawConfirmationDialog(
    ConfirmationDialog& dialog,
    int screenWidth,
    int screenHeight);

}  // namespace UI

#endif
