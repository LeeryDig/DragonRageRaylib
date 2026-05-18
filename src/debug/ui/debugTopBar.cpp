#include "debug/ui/debugUi.hpp"

#include <cstdio>

#include "debug/ui/debugWidgets.hpp"

namespace debug_ui {
namespace {

void SetVectorInput(std::string inputs[3], Vector3 value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.x);
    inputs[0] = buffer;
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.y);
    inputs[1] = buffer;
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.z);
    inputs[2] = buffer;
}

}  // namespace

void DrawTopBar(GameWorld& gameWorld, const TopBarActions& actions) {
    if (!gameWorld.debugUi.enabled) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), 34, Color{28, 28, 32, 235});
    DrawRectangleLines(0, 0, GetScreenWidth(), 34, Color{70, 70, 78, 255});

    const char* items[] = {"Game", "Level", "Inspector", "Debug", "Person", "Physics"};
    int menuX[6] = {};
    int x = 10;
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < 6; ++i) {
        int width = MeasureText(items[i], 20) + 28;
        menuX[i] = x;
        Rectangle rect = Rectangle{static_cast<float>(x), 5.0f, static_cast<float>(width), 24.0f};
        bool hovered = CheckCollisionPointRec(mouse, rect);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.activeMenu = gameWorld.debugUi.activeMenu == i ? -1 : i;
        }
        DrawRectangleRec(rect, hovered || gameWorld.debugUi.activeMenu == i ? Color{62, 62, 70, 255} : Color{42, 42, 48, 255});
        DrawText(items[i], x + 14, 8, 20, RAYWHITE);
        x += width + 6;
    }

    if (gameWorld.debugUi.activeMenu == 0) {
        float dx = static_cast<float>(menuX[0]);
        if (DebugMenuItem(Rectangle{dx, 38, 190, 30}, "Restart Level")) {
            if (actions.restartLevel) actions.restartLevel(gameWorld);
            gameWorld.debugUi.activeMenu = -1;
        }
        if (DebugMenuItem(Rectangle{dx, 68, 190, 30}, "Reset Person")) {
            if (actions.resetGameWorld) actions.resetGameWorld(gameWorld);
            gameWorld.debugUi.activeMenu = -1;
        }
        if (DebugMenuItem(Rectangle{dx, 98, 190, 30}, "Teleport...")) {
            gameWorld.debugUi.gameTeleportOpen = true;
            SetVectorInput(gameWorld.debugUi.gameTeleportInput, gameWorld.person.position);
            gameWorld.debugUi.activeMenu = -1;
        }
    } else if (gameWorld.debugUi.activeMenu == 1) {
        gameWorld.debugUi.levelConfigOpen = true;
        gameWorld.debugUi.levelSidebarOpen = false;
        gameWorld.debugUi.levelLoadOpen = false;
        gameWorld.debugUi.activeMenu = -1;
    } else if (gameWorld.debugUi.activeMenu == 2) {
        gameWorld.debugUi.levelSidebarOpen = true;
        gameWorld.debugUi.levelConfigOpen = false;
        gameWorld.debugUi.levelLoadOpen = false;
        gameWorld.debugUi.activeMenu = -1;
    } else if (gameWorld.debugUi.activeMenu == 3) {
        float dx = static_cast<float>(menuX[3]);
        if (DebugMenuItem(Rectangle{dx, 38, 220, 30}, "Show Forces", true, gameWorld.debugUi.showForces)) {
            gameWorld.debugUi.showForces = !gameWorld.debugUi.showForces;
        }
        if (DebugMenuItem(Rectangle{dx, 68, 220, 30}, "Show Person Status", true, gameWorld.debugUi.showVehicleStatus)) {
            gameWorld.debugUi.showVehicleStatus = !gameWorld.debugUi.showVehicleStatus;
        }
        if (DebugMenuItem(Rectangle{dx, 98, 220, 30}, "Camera Teleport...")) {
            gameWorld.debugUi.debugTeleportOpen = true;
            SetVectorInput(gameWorld.debugUi.debugTeleportInput, gameWorld.camera.position);
            gameWorld.debugUi.activeMenu = -1;
        }
    } else if (gameWorld.debugUi.activeMenu == 4) {
        float dx = static_cast<float>(menuX[4]);
        DebugMenuItem(Rectangle{dx, 38, 220, 30}, "Person Tuning", false);
    } else if (gameWorld.debugUi.activeMenu == 5) {
        float dx = static_cast<float>(menuX[5]);
        if (DebugMenuItem(Rectangle{dx, 38, 220, 30}, "Physics Panel", true, gameWorld.debugUi.showPhysicsPanel)) {
            gameWorld.debugUi.showPhysicsPanel = !gameWorld.debugUi.showPhysicsPanel;
        }
    }

    DrawText(TextFormat("Cam %.2f %.2f %.2f", gameWorld.camera.position.x, gameWorld.camera.position.y, gameWorld.camera.position.z), GetScreenWidth() - 610, 9, 16, LIGHTGRAY);
    DrawText("F1 close menu | Hold RMB + WASD/Q/Z to fly", GetScreenWidth() - 420, 9, 16, LIGHTGRAY);
}

}  // namespace debug_ui
