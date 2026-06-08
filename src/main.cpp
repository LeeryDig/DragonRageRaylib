#include <raylib.h>

#include "debug/cameraDebug.hpp"
#include "game/worldConfig.hpp"
#include "gameplay/gameplayRender.hpp"
#include "gameplay/gameplayUpdate.hpp"
#include "gameplay/worldActions.hpp"
#include "personController.hpp"
#include "uiText.hpp"
#include "utils.hpp"

static const char* WORLD_CONFIG_PATH = "resources/config/world.json";

int main() {
    WorldConfig wc = LoadWorldConfig(
        Utils::ResolveProjectPath(WORLD_CONFIG_PATH), DefaultWorldConfig());

    if (wc.fullscreen) {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
    }
    InitWindow(wc.windowWidth, wc.windowHeight, wc.windowTitle.c_str());
    InitAudioDevice();
    SetExitKey(KEY_NULL);
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    if (!wc.fullscreen && monitorWidth > 0 && monitorHeight > 0) {
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowPosition(0, 0);
    }
    SetTargetFPS(wc.targetFps);
    LoadUiFont();

    GameWorld gameWorld = LoadGameWorld();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F1)) {
            gameWorld.debugUi.enabled = !gameWorld.debugUi.enabled;
            if (gameWorld.debugUi.enabled) {
                gameWorld.debugUi.freeCameraActive = false;
                EnableCursor();
                SyncDebugCameraRotation(gameWorld.render.debugCamera, gameWorld.render.camera);
            } else {
                gameWorld.debugUi.freeCameraActive = false;
                DisableCursor();
                ApplyPersonCamera(
                    gameWorld.render.camera,
                    gameWorld.player.state,
                    gameWorld.player.config,
                    1.0f / static_cast<float>(wc.targetFps));
            }
        }

        UpdateGameplay(gameWorld, GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);
        DrawGameplay(gameWorld);
        EndDrawing();
    }

    UnloadGameWorld(gameWorld);
    UnloadUiFont();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
