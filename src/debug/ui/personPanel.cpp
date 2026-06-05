#include "personPanel.hpp"

#include <algorithm>
#include <raylib.h>

#include "uiText.hpp"
#include <raymath.h>

#include "debug/ui/debugWidgets.hpp"
#include "gameplay/smoking/smokingConfig.hpp"

namespace debug_ui {
namespace {

constexpr float PANEL_WIDTH = 390.0f;
constexpr float ROW_H       = 32.0f;
constexpr int   BASE_FIELD  = 1000;

void DrawSectionHeader(float x, float& y, float width, const char* label) {
    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), 22, Color{50, 54, 65, 255});
    DrawUiText(label, static_cast<int>(x + 10.0f), static_cast<int>(y + 4.0f), 15, Color{180, 190, 210, 255});
    y += 26.0f;
}

void DrawSeparator(float x, float y, float width) {
    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), 1, Color{65, 65, 75, 255});
}

bool DrawToggle(float x, float& y, float width, const char* label, bool& value) {
    Rectangle rect = {x + 10.0f, y, width - 20.0f, ROW_H - 4.0f};
    Vector2 mouse  = GetMousePosition();
    bool hovered   = CheckCollisionPointRec(mouse, rect);
    bool clicked   = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    if (clicked) value = !value;

    DrawRectangleRec(rect, hovered ? Color{62, 62, 72, 255} : Color{36, 36, 44, 255});
    DrawRectangleLinesEx(rect, 1.0f, Color{80, 80, 90, 255});
    Color boxColor = value ? Color{80, 140, 80, 255} : Color{60, 60, 68, 255};
    DrawRectangle(static_cast<int>(rect.x + 6.0f), static_cast<int>(rect.y + 5.0f), 14, 14, boxColor);
    DrawRectangleLines(static_cast<int>(rect.x + 6.0f), static_cast<int>(rect.y + 5.0f), 14, 14, Color{100, 100, 110, 255});
    if (value) DrawUiText("x", static_cast<int>(rect.x + 9.0f), static_cast<int>(rect.y + 4.0f), 16, RAYWHITE);
    DrawUiText(label, static_cast<int>(rect.x + 28.0f), static_cast<int>(rect.y + 6.0f), 16, RAYWHITE);
    y += ROW_H;
    return clicked;
}

bool FieldRow(float x, float& y, float w, const char* label, float& val, float lo, float hi, int id, DebugUiState& ui) {
    bool changed = DebugFloatInputRow({x, y, w, ROW_H}, label, val, lo, hi, id, ui);
    y += ROW_H;
    return changed;
}

void DrawMovementTab(GameWorld& gw, float x, float contentY, float h) {
    PersonConfig& cfg = gw.player.config;
    DebugUiState& ui  = gw.debugUi;
    float w = PANEL_WIDTH - 20.0f;

    float rowCount  = 8.0f;
    float maxScroll = std::max(0.0f, rowCount * ROW_H + 40.0f - (h - contentY));
    if (CheckCollisionPointRec(GetMousePosition(), Rectangle{x, contentY, PANEL_WIDTH, h - contentY}))
        gw.debugUi.personPanelScroll -= static_cast<int>(GetMouseWheelMove() * 3);
    gw.debugUi.personPanelScroll = static_cast<int>(Clamp(static_cast<float>(gw.debugUi.personPanelScroll), 0.0f, maxScroll));

    float y = contentY - static_cast<float>(gw.debugUi.personPanelScroll) + 6.0f;
    int   id = BASE_FIELD;

    bool changed = false;
    changed |= FieldRow(x + 10.0f, y, w, "Walk speed",    cfg.walkSpeed,              0.0f,  20.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Acceleration",  cfg.acceleration,           0.0f,  60.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Deceleration",  cfg.deceleration,           0.0f,  60.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Turn speed",    cfg.turnSpeed,              0.0f,  20.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Mouse sens",    cfg.cameraMouseSensitivity, 0.0f,   0.01f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Camera smooth", cfg.cameraSmooth,           0.0f,  30.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Eye height",    cfg.eyeHeight,              0.0f,   3.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Interact dist", cfg.interactionDistance,    0.0f,  10.0f, id++, ui);
    if (changed) gw.debugUi.configDirty = true;
}

void DrawSmokingTab(GameWorld& gw, float x, float contentY, float h) {
    SmokingConfig& cfg = gw.player.smokingConfig;
    DebugUiState&  ui  = gw.debugUi;
    float w = PANEL_WIDTH - 20.0f;

    float rowCount  = 5.0f + 1.0f + 15.0f + 1.0f;  // cigarro + sep + particles(12 + 3 offsets)
    float totalH    = 26.0f * 2.0f + rowCount * ROW_H + 50.0f;
    float maxScroll = std::max(0.0f, totalH - (h - contentY));
    if (CheckCollisionPointRec(GetMousePosition(), Rectangle{x, contentY, PANEL_WIDTH, h - contentY}))
        gw.debugUi.personPanelScroll -= static_cast<int>(GetMouseWheelMove() * 3);
    gw.debugUi.personPanelScroll = static_cast<int>(Clamp(static_cast<float>(gw.debugUi.personPanelScroll), 0.0f, maxScroll));

    float y  = contentY - static_cast<float>(gw.debugUi.personPanelScroll) + 6.0f;
    int   id = BASE_FIELD + 100;

    bool changed = false;

    // ─── Cigarro ─────────────────────────────────────────────────────────────
    DrawSectionHeader(x, y, PANEL_WIDTH, "Cigarro");
    {
        float packF = static_cast<float>(cfg.defaultPackSize);
        if (DebugFloatInputRow({x + 10.0f, y, w, ROW_H}, "Pack size", packF, 1.0f, 40.0f, id++, ui)) {
            cfg.defaultPackSize = static_cast<int>(packF);
            changed = true;
        }
        y += ROW_H;
    }
    changed |= FieldRow(x + 10.0f, y, w, "Drain idle",    cfg.drainIdle,            0.0f,  0.1f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Drain puff",    cfg.drainPuff,            0.0f,  0.5f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Puff duration", cfg.puffDurationBase,     0.5f,  5.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Puff variance", cfg.puffDurationVariance, 0.0f,  2.0f,  id++, ui);

    // ─── Particulas ───────────────────────────────────────────────────────────
    y += 4.0f;
    DrawSeparator(x, y, PANEL_WIDTH);
    y += 8.0f;
    DrawSectionHeader(x, y, PANEL_WIDTH, "Particulas (pos-tragada)");

    if (DrawToggle(x, y, PANEL_WIDTH, "Emitir particulas", cfg.emitParticles)) changed = true;

    ParticleEmitterConfig& p = cfg.postPuffParticles;
    changed |= FieldRow(x + 10.0f, y, w, "Emit duration", p.emitDuration, 0.1f,  6.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Spawn rate",    p.spawnRate,    1.0f, 60.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Lifetime",      p.lifetime,     0.1f,  6.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Start size",    p.startSize,    0.01f, 2.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "End size",      p.endSize,      0.01f, 2.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Start alpha",   p.startAlpha,   0.0f, 255.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "End alpha",     p.endAlpha,     0.0f, 255.0f, id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Vel Y min",     p.velMinY,      0.0f,  5.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Vel Y max",     p.velMaxY,      0.0f,  5.0f,  id++, ui);
    if (FieldRow(x + 10.0f, y, w, "Drift XZ",             p.velMaxX,      0.0f,  3.0f,  id++, ui)) {
        p.velMinX = -p.velMaxX; p.velMinZ = -p.velMaxX; p.velMaxZ = p.velMaxX;
        changed = true;
    }
    changed |= FieldRow(x + 10.0f, y, w, "Drag",          p.drag,         0.0f,  3.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Offset X",      p.emitOffsetX, -3.0f,  3.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Offset Y",      p.emitOffsetY, -0.5f,  4.0f,  id++, ui);
    changed |= FieldRow(x + 10.0f, y, w, "Offset Z",      p.emitOffsetZ, -3.0f,  3.0f,  id++, ui);

    if (changed) gw.debugUi.configDirty = true;

    if (!cfg.emitParticles) {
        float blockTop = contentY + 26.0f + 26.0f + 6.0f * ROW_H + 4.0f + 8.0f + 26.0f + ROW_H
                         - static_cast<float>(gw.debugUi.personPanelScroll);
        float blockBot = y - 8.0f;
        float blockH   = blockBot - blockTop;
        if (blockH > 0.0f)
            DrawRectangle(static_cast<int>(x), static_cast<int>(blockTop),
                          static_cast<int>(PANEL_WIDTH), static_cast<int>(blockH),
                          Color{0, 0, 0, 90});
    }
}

}  // namespace

void DrawPersonPanel(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled || !gameWorld.debugUi.personPanelOpen) return;

    float x = static_cast<float>(GetScreenWidth()) - PANEL_WIDTH;
    float h = static_cast<float>(GetScreenHeight()) - 34.0f;
    Rectangle panel = {x, 34.0f, PANEL_WIDTH, h};

    DrawRectangleRec(panel, Color{24, 24, 30, 235});
    DrawRectangleLinesEx(panel, 1.0f, Color{80, 80, 88, 255});
    DrawUiText("Person", static_cast<int>(x + 14.0f), 44, 20, RAYWHITE);

    const char* tabs[] = {"Movement", "Smoking"};
    for (int i = 0; i < 2; ++i) {
        Rectangle tab = {x + 12.0f + i * 96.0f, 74.0f, 90.0f, 26.0f};
        bool active  = gameWorld.debugUi.personPanelTab == i;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), tab);
        DrawRectangleRec(tab, active ? Color{78, 92, 120, 255} : (hovered ? Color{64, 64, 72, 255} : Color{42, 42, 48, 255}));
        DrawRectangleLinesEx(tab, 1.0f, Color{85, 85, 95, 255});
        DrawUiText(tabs[i], static_cast<int>(tab.x + 8.0f), static_cast<int>(tab.y + 6.0f), 14, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.personPanelTab    = i;
            gameWorld.debugUi.personPanelScroll = 0;
        }
    }

    float contentY = 108.0f;
    BeginScissorMode(static_cast<int>(x), static_cast<int>(contentY),
                     static_cast<int>(PANEL_WIDTH), static_cast<int>(h - contentY + 34.0f));

    if (gameWorld.debugUi.personPanelTab == 0)
        DrawMovementTab(gameWorld, x, contentY, h + 34.0f);
    else
        DrawSmokingTab(gameWorld, x, contentY, h + 34.0f);

    EndScissorMode();
}

}  // namespace debug_ui
