#include <raylib.h>
#include <raymath.h>

#include <string>
#include <vector>

#include "utils.hpp"
#include "roads.hpp"
#include "scenery.hpp"

#include "gameState.hpp"

SysState sysState = SysState::PLAYING;

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;

struct Car {
    Model model;
    Vector3 cameraPos;
    Vector3 position;
    Vector3 size;
    Vector3 axis;
    float rotation;
    float fuel, consumption, crusingSpeed, maxSpeed, health, velocity;
};

struct Game {
    float distance;
    float finishLine;
};

float FuelConsume(float velocity, float consumption) {
    return velocity * consumption;
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
    car.crusingSpeed = 0.55;
    car.velocity = 0.85;
    car.cameraPos = {0.0f, 0.22f, 0.1f};

    Camera camera = {car.cameraPos, Vector3{0.0f, 0.4f, -1.0f}, Vector3{0.0f, 1.0f, 0.0f}, 50.0f, 0};

    Game game;
    game.distance = 0.0f;

    std::vector<Roads> roads = GenerateRoads(30, -30.0);
    std::vector<Scenery> buildingsRight = GenerateScenery(15, 10, 50, 10, 0);
    std::vector<Scenery> buildingsLeft = GenerateScenery(15, 10, 50, 10, 1);

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        // DisableCursor();
        game.distance += car.velocity * GetFrameTime();

        if (car.velocity < 0.05) {
            car.velocity = 0.0f;
        }

        if (car.fuel > 0.0) {
            car.fuel -= FuelConsume(car.velocity, car.consumption);
            car.velocity = Clamp(car.velocity, car.crusingSpeed, car.maxSpeed);
        } else {
            car.velocity = Lerp(car.velocity, 0.0f, 0.005);
        }

        if (IsKeyDown(KEY_UP) && car.fuel > 0.0) {
            car.velocity += Lerp(0.0, 0.1, 0.14);
        } else if (IsKeyDown(KEY_DOWN) && car.fuel > 0.0) {
            car.velocity -= Lerp(0.0, 0.05, 0.1);
        } else if (car.velocity >= car.crusingSpeed) {
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

        // Draw
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModelEx(car.model, car.position, car.axis, car.rotation, car.size, WHITE);
        DrawCube(Vector3{0.0, 0.0, 0.0}, 100, 0.0, 100, DARKGREEN);

        UpdateScenery(buildingsLeft, car.velocity);
        DrawScenery(buildingsLeft, game.distance);
        UpdateScenery(buildingsRight, car.velocity);
        DrawScenery(buildingsRight, game.distance);

        UpdateRoads(roads, car.velocity);
        DrawRoads(roads);

        EndMode3D();

        if (car.velocity < 0.1f || car.health <= 0.0f) {
            DrawText("Acabou a gasosa", 400, 360, 50, VIOLET);
        }

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
