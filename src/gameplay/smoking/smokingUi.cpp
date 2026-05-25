#include "smokingUi.hpp"

#include <string>

#include <raylib.h>

namespace {
    constexpr float BAR_WIDTH    = 200.0f;
    constexpr float BAR_HEIGHT   = 14.0f;
    constexpr float BAR_Y_OFFSET = 60.0f;   // from bottom
    constexpr float PACK_GAP     = 24.0f;   // gap above bar
    constexpr int   FONT_SIZE    = 18;
}

void DrawSmokingUi(const SmokingState& state, int screenWidth, int screenHeight) {
    float barX = (screenWidth  - BAR_WIDTH) * 0.5f;
    float barY =  screenHeight - BAR_Y_OFFSET - BAR_HEIGHT;

    std::string packText = "Cigarros: " + std::to_string(state.cigarettesInPack);
    int textWidth = MeasureText(packText.c_str(), FONT_SIZE);
    DrawText(
        packText.c_str(),
        screenWidth / 2 - textWidth / 2,
        static_cast<int>(barY - PACK_GAP - FONT_SIZE),
        FONT_SIZE,
        LIGHTGRAY);

    if (state.phase == SmokingPhase::NONE) return;

    Rectangle track = {barX, barY, BAR_WIDTH, BAR_HEIGHT};
    DrawRectangleRec(track, Color{40, 40, 40, 200});
    DrawRectangleLinesEx(track, 1.0f, Color{80, 80, 80, 200});

    float fillW = BAR_WIDTH * state.cigaretteLife;
    if (fillW > 0.0f) {
        Color fillColor = (state.phase == SmokingPhase::PUFFING)
            ? Color{255, 200, 80, 255}
            : Color{210, 120, 40, 255};
        DrawRectangleRec({barX, barY, fillW, BAR_HEIGHT}, fillColor);
    }

    const char* label = (state.phase == SmokingPhase::PUFFING) ? "Tragando..." : "Aceso";
    DrawText(label, static_cast<int>(barX + BAR_WIDTH + 10.0f), static_cast<int>(barY), 14, DARKGRAY);
}
