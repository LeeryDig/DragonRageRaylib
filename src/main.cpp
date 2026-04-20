#include <raylib.h>
#include <raymath.h>

#include <cstddef>
#include <string>
#include <vector>

#include "debug/cameraDebug.hpp"
#include "gameState.hpp"
#include "roads.hpp"
#include "scenery.hpp"
#include "ui.hpp"
#include "utils.hpp"

SysState sysState = SysState::MENU;

namespace {

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;
const int MENU_BUTTON_WIDTH = 300;
const int MENU_BUTTON_HEIGHT = 58;
const int MENU_BUTTON_SPACING = 22;
const int MENU_START_Y = 280;
const char* CAMERA_CONFIG_PATH = "resources/config/camera.json";

struct Car {
    Model model;
    Texture2D texture;
    Vector3 position;
    Vector3 size;
    Vector3 axis;
    float rotation;
    float fuel;
    float consumption;
    float crusingSpeed;
    float maxSpeed;
    float health;
    float velocity;
};

struct Game {
    float distance;
    float finishLine;
};

struct GameWorld {
    Car car;
    Camera camera;
    CockpitCameraConfig cockpitCamera;
    DebugCameraState debugCamera;
    Game game;
    std::vector<Roads> roads;
    std::vector<Scenery> leftScenery;
    std::vector<Scenery> rightScenery;
};

float FuelConsume(float velocity, float consumption) {
    return velocity * consumption;
}

Rectangle MakeCenteredButton(float topY) {
    return Rectangle{
        static_cast<float>((SCRWIDTH - MENU_BUTTON_WIDTH) / 2),
        topY,
        static_cast<float>(MENU_BUTTON_WIDTH),
        static_cast<float>(MENU_BUTTON_HEIGHT)
    };
}

void ConfigureCarDefaults(Car& car) {
    car.position = {0.0f, 0.1f, 0.0f};
    car.size = {1.0f, 1.0f, 1.0f};
    car.axis = {0.0f, 1.0f, 0.0f};
    car.rotation = 0.0f;
    car.health = 10.0f;
    car.fuel = 60.0f;
    car.consumption = 0.005f;
    car.maxSpeed = 2.0f;
    car.crusingSpeed = 0.55f;
    car.velocity = 0.85f;
}

void ResetRoadPositions(std::vector<Roads>& roads) {
    float zPos = -30.0f;
    for (auto& road : roads) {
        road.position = Vector3{0.0f, 0.001f, zPos};
        zPos += road.length;
    }
}

void ResetSceneryPositions(std::vector<Scenery>& scenery, float xPos, float frequency) {
    if (scenery.empty()) {
        return;
    }

    BoundingBox bbox = GetModelBoundingBox(scenery[0].model);
    float zPos = -30.0f;
    float length = bbox.max.z - bbox.min.z;

    for (auto& object : scenery) {
        object.position = Vector3{xPos, 0.0f, zPos};
        object.scale = 1.0f;
        zPos += length + frequency;
    }
}

void ResetGameWorld(GameWorld& world) {
    ConfigureCarDefaults(world.car);
    world.debugCamera.enabled = false;
    ApplyCockpitCamera(world.camera, world.car.position, world.cockpitCamera);
    world.game.distance = 0.0f;
    world.game.finishLine = 0.0f;
    ResetRoadPositions(world.roads);
    ResetSceneryPositions(world.leftScenery, -5.0f, 3.0f);
    ResetSceneryPositions(world.rightScenery, 5.0f, 3.5f);
}

void LoadGameWorld(GameWorld& world) {
    CockpitCameraConfig fallbackCockpitCamera = {
        Vector3{0.0f, 0.22f, 0.1f},
        Vector3{0.0f, 0.4f, -1.0f},
        50.0f
    };
    DebugCameraState fallbackDebugCamera = {
        false,
        8.0f,
        0.003f,
        0.0f,
        0.0f
    };

    world.cockpitCamera = LoadCockpitCameraConfig(CAMERA_CONFIG_PATH, fallbackCockpitCamera);
    world.debugCamera = LoadDebugCameraStateConfig(CAMERA_CONFIG_PATH, fallbackDebugCamera);
    world.car.model = LoadModel("resources/models/CockpitF1.glb");
    world.car.texture = LoadTexture("resources/textures/F1CockpitDiff.png");
    SetMaterialTexture(&world.car.model.materials[0], MATERIAL_MAP_DIFFUSE, world.car.texture);

    world.roads = GenerateRoads(30);
    world.leftScenery = GenerateScenery(-5.0f, 10, 3.0f);
    world.rightScenery = GenerateScenery(5.0f, 10, 3.5f);

    ResetGameWorld(world);
}

void UnloadGameWorld(GameWorld& world) {
    for (auto& road : world.roads) {
        UnloadModel(road.model);
        break;
    }

    for (auto& object : world.leftScenery) {
        UnloadModel(object.model);
        break;
    }

    for (auto& object : world.rightScenery) {
        UnloadModel(object.model);
        break;
    }

    UnloadTexture(world.car.texture);
    UnloadModel(world.car.model);
}

void UpdateGameplay(GameWorld& world) {
    world.game.distance += world.car.velocity * GetFrameTime();

    if (world.car.velocity < 0.05f) {
        world.car.velocity = 0.0f;
    }

    if (world.car.fuel > 0.0f) {
        world.car.fuel -= FuelConsume(world.car.velocity, world.car.consumption);
        world.car.velocity = Clamp(world.car.velocity, world.car.crusingSpeed, world.car.maxSpeed);
    } else {
        world.car.velocity = Lerp(world.car.velocity, 0.0f, 0.005f);
    }

    if (!world.debugCamera.enabled) {
        if (IsKeyDown(KEY_UP) && world.car.fuel > 0.0f) {
            world.car.velocity += Lerp(0.0f, 0.1f, 0.14f);
        } else if (IsKeyDown(KEY_DOWN) && world.car.fuel > 0.0f) {
            world.car.velocity -= Lerp(0.0f, 0.05f, 0.1f);
        } else if (world.car.velocity >= world.car.crusingSpeed) {
            world.car.velocity -= Lerp(0.0f, 0.01f, 0.05f);
        }

        float radialRotation = Utils::ConvertAngleToRadial(world.car.rotation);
        float amount = Clamp(cosf(radialRotation) * 0.1f, -0.2f, 0.2f);

        if (IsKeyDown(KEY_RIGHT) && world.car.position.x <= 2.8f) {
            world.car.rotation -= 0.5f;
            world.car.position.x += amount;
        } else if (IsKeyDown(KEY_LEFT) && world.car.position.x >= -2.8f) {
            world.car.rotation += 0.5f;
            world.car.position.x -= amount;
        } else {
            world.car.rotation = Lerp(world.car.rotation, 0.0f, 0.07f);
        }

        ApplyCockpitCamera(world.camera, world.car.position, world.cockpitCamera);
    } else {
        UpdateDebugCamera(world.debugCamera, world.camera);
    }

    UpdateScenery(world.leftScenery, world.car.velocity);
    UpdateScenery(world.rightScenery, world.car.velocity);
    UpdateRoads(world.roads, world.car.velocity);
}

void DrawGameplay(const GameWorld& world) {
    BeginMode3D(world.camera);

    DrawModelEx(world.car.model, world.car.position, world.car.axis, world.car.rotation, world.car.size, WHITE);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 100.0f, 0.0f, 100.0f, DARKGREEN);
    DrawScenery(world.leftScenery);
    DrawScenery(world.rightScenery);
    DrawRoads(world.roads);

    EndMode3D();

    if (world.car.velocity < 0.1f || world.car.health <= 0.0f) {
        DrawText("Acabou a gasosa", 400, 360, 50, VIOLET);
    }

    DrawText(std::to_string(static_cast<int>(world.car.velocity * 100.0f)).c_str(), 600, 500, 20, BLACK);
    DrawText("DISTANCE", 1000, 80, 20, BLACK);
    DrawText(std::to_string(static_cast<int>(world.game.distance)).c_str(), 1000, 100, 20, WHITE);
    DrawText("Health: ", 20, 50, 20, BLACK);
    DrawText(std::to_string(static_cast<int>(world.car.health)).c_str(), 100, 50, 20, BLACK);
    DrawText("Fuel: ", 20, 80, 20, BLACK);
    DrawText(std::to_string(static_cast<int>(world.car.fuel)).c_str(), 80, 80, 20, BLACK);
    DrawText(world.debugCamera.enabled ? "DEBUG CAMERA [F1]" : "COCKPIT [F1]", 20, 110, 20, BLACK);
    DrawFPS(10, 10);
}

void DrawMenuTitle() {
    const char* title = "Dragon Rage";
    int fontSize = 48;
    int textWidth = MeasureText(title, fontSize);
    DrawText(title, (SCRWIDTH - textWidth) / 2, 150, fontSize, BLACK);
    DrawText("Selecione uma opcao", (SCRWIDTH - MeasureText("Selecione uma opcao", 24)) / 2, 210, 24, DARKGRAY);
}

void DrawOptionsScreen(bool buttonsEnabled) {
    const char* title = "Opcoes";
    DrawText(title, (SCRWIDTH - MeasureText(title, 44)) / 2, 160, 44, BLACK);
    DrawText("Aqui sera as opcoes", (SCRWIDTH - MeasureText("Aqui sera as opcoes", 28)) / 2, 280, 28, DARKGRAY);

    Rectangle backButton = MakeCenteredButton(390.0f);
    if (UI::DrawButton(backButton, "Voltar", buttonsEnabled)) {
        sysState = SysState::MENU;
    }
}

}  // namespace

int main() {
    InitWindow(SCRWIDTH, SCRHEIGHT, "SPEED");
    SetTargetFPS(60);

    GameWorld world = {};
    UI::ConfirmationDialog exitDialog = {};
    bool shouldExit = false;

    LoadGameWorld(world);

    while (!WindowShouldClose() && !shouldExit) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        bool backgroundInputEnabled = !exitDialog.isOpen;
        Rectangle startButton = MakeCenteredButton(static_cast<float>(MENU_START_Y));
        Rectangle optionsButton = MakeCenteredButton(static_cast<float>(MENU_START_Y + MENU_BUTTON_HEIGHT + MENU_BUTTON_SPACING));
        Rectangle exitButton = MakeCenteredButton(static_cast<float>(MENU_START_Y + (MENU_BUTTON_HEIGHT + MENU_BUTTON_SPACING) * 2));

        switch (sysState) {
            case SysState::MENU:
                DrawMenuTitle();

                if (UI::DrawButton(startButton, "Iniciar Jogo", backgroundInputEnabled)) {
                    ResetGameWorld(world);
                    sysState = SysState::PLAYING;
                }

                if (UI::DrawButton(optionsButton, "Opcoes", backgroundInputEnabled)) {
                    sysState = SysState::OPTIONS;
                }

                if (UI::DrawButton(exitButton, "Sair", backgroundInputEnabled)) {
                    UI::OpenConfirmationDialog(
                        exitDialog,
                        "Sair do jogo",
                        "Deseja realmente sair?",
                        "Sim",
                        "Nao");
                }
                break;

            case SysState::OPTIONS:
                DrawOptionsScreen(backgroundInputEnabled);
                break;

            case SysState::PLAYING:
                if (backgroundInputEnabled && IsKeyPressed(KEY_F1)) {
                    world.debugCamera.enabled = !world.debugCamera.enabled;

                    if (world.debugCamera.enabled) {
                        DisableCursor();
                        SyncDebugCameraRotation(world.debugCamera, world.camera);
                    } else {
                        EnableCursor();
                        ApplyCockpitCamera(world.camera, world.car.position, world.cockpitCamera);
                    }
                }

                if (backgroundInputEnabled) {
                    UpdateGameplay(world);
                }
                DrawGameplay(world);
                break;

            case SysState::EDITOR:
            case SysState::PAUSED:
                break;
        }

        UI::ModalResult exitResult = UI::DrawConfirmationDialog(exitDialog, SCRWIDTH, SCRHEIGHT);
        if (exitResult == UI::ModalResult::CONFIRMED) {
            shouldExit = true;
        }

        EndDrawing();
    }

    EnableCursor();
    UnloadGameWorld(world);
    CloseWindow();

    return 0;
}
