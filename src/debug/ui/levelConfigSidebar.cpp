#include "debug/ui/debugUi.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

#include "raymath.h"

#include "debug/ui/debugWidgets.hpp"
#include "level/levelsConfig.hpp"
#include "utils.hpp"

namespace debug_ui {
namespace {

bool IsSkyboxFile(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) return false;
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".hdr" || ext == ".ktx";
}

std::string FileNameFromPath(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string DisplayNameFromPath(const std::string& path) {
    std::filesystem::path filePath(path);
    std::string stem = filePath.stem().string();
    return stem.empty() ? FileNameFromPath(path) : stem;
}

std::string LevelEntryLabel(const LevelConfigEntry& entry) {
    std::string display = GetLevelDisplayName(entry);
    return DisplayNameFromPath(display.empty() ? entry.path : display);
}

std::vector<std::string> ListSkyboxPaths() {
    std::vector<std::string> paths;
    const std::string rootRelative = "resources/assets/skybox";
    std::filesystem::path root = Utils::ResolveProjectPath(rootRelative);
    if (!std::filesystem::exists(root)) return paths;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root)) {
        if (!IsSkyboxFile(entry.path())) continue;
        paths.push_back(rootRelative + "/" + entry.path().filename().string());
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

}  // namespace

void DrawLevelConfigSidebar(GameWorld& gameWorld, const LevelConfigActions& actions) {
    if (!gameWorld.debugUi.enabled || !gameWorld.debugUi.levelConfigOpen) return;

    float width = 390.0f;
    float x = static_cast<float>(GetScreenWidth()) - width;
    float h = static_cast<float>(GetScreenHeight()) - 34.0f;
    Rectangle panel = Rectangle{x, 34.0f, width, h};
    DrawRectangleRec(panel, Color{24, 24, 30, 235});
    DrawRectangleLinesEx(panel, 1.0f, Color{80, 80, 88, 255});
    DrawText("Level", static_cast<int>(x + 14.0f), 44, 20, RAYWHITE);

    const char* tabs[] = {"Skyboxes", "Fog", "Levels"};
    for (int i = 0; i < 3; ++i) {
        Rectangle tab = Rectangle{x + 12.0f + i * 96.0f, 74.0f, 90.0f, 26.0f};
        bool active = gameWorld.debugUi.levelConfigTab == i;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), tab);
        DrawRectangleRec(tab, active ? Color{78, 92, 120, 255} : hovered ? Color{64, 64, 72, 255} : Color{42, 42, 48, 255});
        DrawRectangleLinesEx(tab, 1.0f, Color{85, 85, 95, 255});
        DrawText(tabs[i], static_cast<int>(tab.x + 8.0f), static_cast<int>(tab.y + 6.0f), 14, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.levelConfigTab = i;
            gameWorld.debugUi.levelConfigScroll = 0;
            gameWorld.debugUi.levelLoadScroll = 0;
        }
    }

    float y = 116.0f;
    if (gameWorld.debugUi.levelConfigTab == 0) {
        std::string configLabel = gameWorld.currentLevelConfigPath.empty() ? "No .json for current level" : FileNameFromPath(gameWorld.currentLevelConfigPath);
        DrawText(configLabel.c_str(), static_cast<int>(x + 14.0f), static_cast<int>(y), 14, GRAY);
        y += 28.0f;

        std::vector<std::string> skyboxes = ListSkyboxPaths();
        int count = static_cast<int>(skyboxes.size());
        if (count == 0) {
            DrawText("No skybox files in resources/assets/skybox", static_cast<int>(x + 14.0f), static_cast<int>(y), 14, ORANGE);
            return;
        }

        float rowH = 38.0f;
        int visibleRows = std::min(count, std::max(1, static_cast<int>((h - 220.0f) / rowH)));
        if (CheckCollisionPointRec(GetMousePosition(), Rectangle{x, y, width, visibleRows * rowH})) {
            gameWorld.debugUi.levelConfigScroll -= static_cast<int>(GetMouseWheelMove());
        }
        gameWorld.debugUi.levelConfigScroll = Clamp(gameWorld.debugUi.levelConfigScroll, 0, std::max(0, count - visibleRows));

        for (int i = 0; i < visibleRows; ++i) {
            int skyboxIndex = gameWorld.debugUi.levelConfigScroll + i;
            const std::string& path = skyboxes[static_cast<std::size_t>(skyboxIndex)];
            Rectangle row = Rectangle{x + 12.0f, y + i * rowH, width - 24.0f, rowH - 4.0f};
            bool current = path == gameWorld.currentLevelRuntimeConfig.skyboxPath;
            bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
            DrawRectangleRec(row, current ? Color{80, 100, 80, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
            DrawRectangleLinesEx(row, 1.0f, current ? GREEN : Color{72, 72, 80, 255});
            std::string label = FileNameFromPath(path);
            DrawText(label.c_str(), static_cast<int>(row.x + 8.0f), static_cast<int>(row.y + 10.0f), 14, RAYWHITE);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !current && !gameWorld.currentLevelConfigPath.empty()) {
                gameWorld.currentLevelRuntimeConfig.skyboxPath = path;
                if (actions.saveCurrentLevelRuntimeConfig) actions.saveCurrentLevelRuntimeConfig(gameWorld);
                if (actions.reloadCurrentLevelForConfig) actions.reloadCurrentLevelForConfig(gameWorld);
            }
        }

        float bottomY = y + visibleRows * rowH + 12.0f;
        std::string currentLabel = gameWorld.currentLevelRuntimeConfig.skyboxPath.empty() ? "(none)" : FileNameFromPath(gameWorld.currentLevelRuntimeConfig.skyboxPath);
        DrawText(TextFormat("Current: %s", currentLabel.c_str()), static_cast<int>(x + 14.0f), static_cast<int>(bottomY), 14, LIGHTGRAY);
        return;
    }

    if (gameWorld.debugUi.levelConfigTab == 1) {
        FogConfig& fog = gameWorld.currentLevelRuntimeConfig.fog;
        DrawText("Simple linear distance fog. Skybox stays clean.", static_cast<int>(x + 14.0f), static_cast<int>(y), 14, GRAY);
        y += 32.0f;

        if (DebugButton(Rectangle{x + 14.0f, y, 120.0f, 28.0f}, fog.enabled ? "Enabled" : "Disabled")) {
            fog.enabled = !fog.enabled;
            gameWorld.debugUi.levelConfigDirty = true;
        }
        if (DebugButton(Rectangle{x + 146.0f, y, 96.0f, 28.0f}, "Save Fog")) {
            if (actions.saveCurrentLevelRuntimeConfig) actions.saveCurrentLevelRuntimeConfig(gameWorld);
        }
        y += 44.0f;

        float red = static_cast<float>(fog.color.r);
        float green = static_cast<float>(fog.color.g);
        float blue = static_cast<float>(fog.color.b);
        if (DebugFloatSlider(Rectangle{x + 14.0f, y, width - 28.0f, 26.0f}, "Red", red, 0.0f, 255.0f)) {
            fog.color.r = static_cast<unsigned char>(Clamp(red, 0.0f, 255.0f));
            gameWorld.debugUi.levelConfigDirty = true;
        }
        y += 32.0f;
        if (DebugFloatSlider(Rectangle{x + 14.0f, y, width - 28.0f, 26.0f}, "Green", green, 0.0f, 255.0f)) {
            fog.color.g = static_cast<unsigned char>(Clamp(green, 0.0f, 255.0f));
            gameWorld.debugUi.levelConfigDirty = true;
        }
        y += 32.0f;
        if (DebugFloatSlider(Rectangle{x + 14.0f, y, width - 28.0f, 26.0f}, "Blue", blue, 0.0f, 255.0f)) {
            fog.color.b = static_cast<unsigned char>(Clamp(blue, 0.0f, 255.0f));
            gameWorld.debugUi.levelConfigDirty = true;
        }
        y += 40.0f;

        if (DebugFloatSlider(Rectangle{x + 14.0f, y, width - 28.0f, 26.0f}, "Start", fog.start, 0.0f, 500.0f)) {
            if (fog.start > fog.end - 1.0f) fog.end = fog.start + 1.0f;
            gameWorld.debugUi.levelConfigDirty = true;
        }
        y += 32.0f;
        if (DebugFloatSlider(Rectangle{x + 14.0f, y, width - 28.0f, 26.0f}, "End", fog.end, 1.0f, 800.0f)) {
            if (fog.end < fog.start + 1.0f) fog.start = fog.end - 1.0f;
            gameWorld.debugUi.levelConfigDirty = true;
        }
        y += 32.0f;
        if (DebugFloatSlider(Rectangle{x + 14.0f, y, width - 28.0f, 26.0f}, "Density", fog.density, 0.25f, 4.0f)) {
            gameWorld.debugUi.levelConfigDirty = true;
        }
        y += 42.0f;

        DrawRectangleRec(Rectangle{x + 14.0f, y, 70.0f, 28.0f}, fog.color);
        DrawRectangleLinesEx(Rectangle{x + 14.0f, y, 70.0f, 28.0f}, 1.0f, RAYWHITE);
        DrawText(TextFormat("RGB %d %d %d", fog.color.r, fog.color.g, fog.color.b), static_cast<int>(x + 96.0f), static_cast<int>(y + 6.0f), 14, LIGHTGRAY);
        y += 42.0f;
        DrawText(TextFormat("Mode: linear  %s", gameWorld.debugUi.levelConfigDirty ? "[unsaved]" : "[saved]"), static_cast<int>(x + 14.0f), static_cast<int>(y), 14, gameWorld.debugUi.levelConfigDirty ? YELLOW : LIGHTGRAY);
        return;
    }

    DrawText("Ordem da lista define level inicial no boot.", static_cast<int>(x + 14.0f), static_cast<int>(y), 14, GRAY);
    y += 28.0f;
    float rowH = 46.0f;
    int count = static_cast<int>(gameWorld.levelsConfig.levels.size());
    if (count == 0) {
        DrawText("No levels in levels.json", static_cast<int>(x + 14.0f), static_cast<int>(y), 16, ORANGE);
        return;
    }

    gameWorld.debugUi.selectedLevelConfigIndex = Clamp(gameWorld.debugUi.selectedLevelConfigIndex, 0, count - 1);
    int visibleRows = std::min(count, std::max(1, static_cast<int>((h - 250.0f) / rowH)));
    if (CheckCollisionPointRec(GetMousePosition(), Rectangle{x, y, width, visibleRows * rowH})) {
        gameWorld.debugUi.levelLoadScroll -= static_cast<int>(GetMouseWheelMove());
    }
    gameWorld.debugUi.levelLoadScroll = Clamp(gameWorld.debugUi.levelLoadScroll, 0, std::max(0, count - visibleRows));
    for (int i = 0; i < visibleRows; ++i) {
        int levelIndex = gameWorld.debugUi.levelLoadScroll + i;
        const LevelConfigEntry& entry = gameWorld.levelsConfig.levels[static_cast<std::size_t>(levelIndex)];
        Rectangle row = Rectangle{x + 12.0f, y + i * rowH, width - 24.0f, rowH - 4.0f};
        bool selected = gameWorld.debugUi.selectedLevelConfigIndex == levelIndex;
        bool current = gameWorld.currentLevelConfigIndex == levelIndex;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
        DrawRectangleRec(row, selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
        DrawRectangleLinesEx(row, 1.0f, current ? GREEN : Color{72, 72, 80, 255});
        std::string levelLabel = LevelEntryLabel(entry);
        DrawText(TextFormat("%d. %s%s", levelIndex + 1, levelLabel.c_str(), current ? "  [loaded]" : ""), static_cast<int>(row.x + 8.0f), static_cast<int>(row.y + 6.0f), 16, RAYWHITE);
        DrawText(TextFormat("Level file: %s", FileNameFromPath(entry.path).c_str()), static_cast<int>(row.x + 8.0f), static_cast<int>(row.y + 25.0f), 13, LIGHTGRAY);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.selectedLevelConfigIndex = levelIndex;
        }
    }

    float actionY = y + visibleRows * rowH + 34.0f;
    int selectedIndex = gameWorld.debugUi.selectedLevelConfigIndex;
    const LevelConfigEntry* selectedEntry = GetLevelConfigEntry(gameWorld.levelsConfig, selectedIndex);
    if (selectedEntry != nullptr) {
        std::string selectedLabel = LevelEntryLabel(*selectedEntry);
        DrawText(TextFormat("Selected: %s", selectedLabel.c_str()), static_cast<int>(x + 14.0f), static_cast<int>(actionY), 16, YELLOW);
        actionY += 24.0f;
        std::string selectedConfigLabel = selectedEntry->configPath.empty() ? "(planned)" : FileNameFromPath(selectedEntry->configPath);
        DrawText(TextFormat("Config: %s", selectedConfigLabel.c_str()), static_cast<int>(x + 14.0f), static_cast<int>(actionY), 14, GRAY);
        actionY += 34.0f;
    }

    if (DebugButton(Rectangle{x + 14.0f, actionY, 110.0f, 28.0f}, "Load")) {
        if (actions.loadConfiguredLevel) actions.loadConfiguredLevel(gameWorld, selectedIndex);
    }
    if (DebugButton(Rectangle{x + 134.0f, actionY, 110.0f, 28.0f}, "Move Up")) {
        if (actions.moveLevelConfigEntry) actions.moveLevelConfigEntry(gameWorld, selectedIndex, selectedIndex - 1);
    }
    if (DebugButton(Rectangle{x + 254.0f, actionY, 110.0f, 28.0f}, "Move Down")) {
        if (actions.moveLevelConfigEntry) actions.moveLevelConfigEntry(gameWorld, selectedIndex, selectedIndex + 1);
    }
}

}  // namespace debug_ui
