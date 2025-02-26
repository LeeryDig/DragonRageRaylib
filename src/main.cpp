#include <raylib.h>
#include <raymath.h>

#include <string>
#include <vector>

#include "utils.hpp"
#include "roads.hpp"

namespace gameState {
#include "gameState.hpp"
}

gameState::SysState sysState = gameState::SysState::PLAYING;

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;

struct Scenery {
    Vector3 pos;
    Vector3 size;
    Color color;
};

struct Car {
    Model model;
    Vector3 cameraPos;
    Vector3 position;
    Vector3 size;
    Vector3 axis;
    float rotation;
    float fuel, consumption, minSpeed, maxSpeed, health, velocity;
};

struct Game {
    float distance;
    float finishLine;
};

float FuelConsume(float velocity, float consumption) {
    return velocity * consumption;
}

std::vector<Scenery> MakeObject(Vector3 position, Vector3 minSize, Vector3 maxSize, Color color, int quantity) {
    std::vector<Scenery> result;
    for (size_t i = 0; i < quantity; i++) {
        float size_x = (float)GetRandomValue((int)minSize.x, (int)maxSize.x);
        float size_y = (float)GetRandomValue((int)minSize.y, (int)maxSize.y);
        float size_z = (float)GetRandomValue((int)minSize.z, (int)maxSize.z);
        Vector3 newPos = {position.x, position.y, position.z -= size_z};
        Scenery scenery = {newPos, {size_x, size_y, size_z}, color};

        result.push_back(scenery);
    }
    return result;
}

void ColorSelection(std::vector<Scenery>& objects, Color color1, Color color2) {
    for (size_t i = 0; i < objects.size(); i++) {
        objects[i].color = (i % 2) ? color1 : color2;
    }
}

int main() {
    InitWindow(SCRWIDTH, SCRHEIGHT, "SPEED");

    Car car;
    car.position = {0.0f, 0.1f, 0.0f};
    car.size = {1.0f, 1.0f, 1.0f};
    car.axis = {0.0f, 1.0f, 0.0f};
    car.model = LoadModel("resources/models/CockpitF1.glb");
    Texture2D texture = LoadTexture("resources/models/F1CockpitDiff.png");
    SetMaterialTexture(&car.model.materials[0], MATERIAL_MAP_DIFFUSE, texture);
    car.rotation = 0.0f;
    car.health = 10;
    car.fuel = 60;
    car.consumption = 0.005;
    car.maxSpeed = 2.0;
    car.minSpeed = 0.5;
    car.velocity = 0.85;
    car.cameraPos = {0.0f, 0.22f, 0.1f};

    Camera camera = {car.cameraPos, (Vector3){0.0f, 0.4f, -1.0f}, (Vector3){0.0f, 1.0f, 0.0f}, 50.0f, 0};

    Game game;
    game.distance = 0.0f;

    std::vector<Roads> roads = GenerateRoads(30, -30.0);

    std::vector<Scenery> rightBuildings = MakeObject((Vector3){25, 0, 0}, (Vector3){10, 25, 10}, (Vector3){15, 60, 15}, DARKBLUE, 10);
    std::vector<Scenery> leftBuildings = MakeObject((Vector3){-25, 0, 0}, (Vector3){10, 25, 10}, (Vector3){15, 60, 15}, DARKBLUE, 10);

    std::vector<Scenery> obstacles = MakeObject((Vector3){0, 0.01, 0}, (Vector3){1, 1.5, 1}, (Vector3){20, 0.1, 10}, GRAY, 13);

    ColorSelection(rightBuildings, DARKBLUE, DARKGRAY);
    ColorSelection(leftBuildings, DARKBLUE, DARKGRAY);

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        // DisableCursor();
        // Update
        game.distance += car.velocity * GetFrameTime();

        if (car.velocity < 0.05) {
            car.velocity = 0.0f;
        }

        // Player Status
        if (car.fuel > 0.0) {
            car.fuel -= FuelConsume(car.velocity, car.consumption);
            car.velocity = Clamp(car.velocity, car.minSpeed, car.maxSpeed);
        } else {
            car.velocity = Lerp(car.velocity, 0.0f, car.consumption);
        }

        // Move player

        if (IsKeyDown(KEY_UP) && car.fuel > 0.0) {
            car.velocity += Lerp(0.0, 0.1, 0.14);
        } else if (IsKeyDown(KEY_DOWN) && car.fuel > 0.0) {
            car.velocity -= Lerp(0.0, 0.05, 0.1);
        } else if (car.velocity >= car.minSpeed) {
            car.velocity -= Lerp(0.0, 0.01, 0.05);
        }

        float radialPRotation = Utils::ConvertAngleToRadial(car.rotation);
        float amout = cosf(radialPRotation) * 0.1f;
        Clamp(amout, -0.2f, 0.2f);

        if (IsKeyDown(KEY_RIGHT) && car.position.x <= 2.8) {
            car.rotation -= 0.5;
            car.position.x += amout;
            camera.position.x += amout;
            camera.target.x += amout;
        } else if (IsKeyDown(KEY_LEFT) && car.position.x >= -2.8) {
            car.rotation += 0.5;
            car.position.x -= amout;
            camera.position.x -= amout;
            camera.target.x -= amout;
        } else {
            car.rotation = Lerp(car.rotation, 0.0f, 0.07f);
        }

        if (IsKeyDown(KEY_E))
            camera.position.y += 0.1f;
        else if (IsKeyDown(KEY_Q))
            camera.position.y -= 0.1f;
        else if (IsKeyDown(KEY_A))
            camera.position.x -= 0.1f;
        else if (IsKeyDown(KEY_D))
            camera.position.x += 0.1f;
        else if (IsKeyDown(KEY_S))
            camera.position.z -= 0.1f;
        else if (IsKeyDown(KEY_W))
            camera.position.z += 0.1f;

        // Draw
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModelEx(car.model, car.position, car.axis, car.rotation, car.size, WHITE);
        DrawCube((Vector3){0.0, 0.0, 0.0}, 100, 0.0, 100, DARKGREEN);
        
        UpdateRoads(roads, car.velocity);
        DrawRoads(roads);

        EndMode3D();

        if (car.velocity < 0.1f || car.health <= 0.0f) {
            DrawText("Acabou a gasosa", 400, 360, 50, VIOLET);
        }

        DrawText("NICE GRAPHICS", 10, 30, 10, BLACK);
        DrawText(std::to_string((int)(car.velocity * 100.0)).c_str(), 600, 500, 20, BLACK);
        DrawText("DISTANCE", 1000, 80, 20, BLACK);
        DrawText(std::to_string((int)game.distance).c_str(), 1000, 100, 20, WHITE);
        DrawText("Health: ", 20, 50, 20, BLACK);
        DrawText(std::to_string(car.health).c_str(), 100, 50, 20, BLACK);
        DrawText("Fuel: ", 20, 80, 20, BLACK);
        DrawText(std::to_string((int)car.fuel).c_str(), 80, 80, 20, BLACK);

        DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
