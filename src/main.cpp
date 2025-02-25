#include <raylib.h>
#include <raymath.h>

#include <string>
#include <vector>

#include "utils.hpp"
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

int main() {
    InitWindow(SCRWIDTH, SCRHEIGHT, "SPEED");

    Car car;
    car.position = {0.0f, 0.2f, 0.0f};
    car.size = {1.0f, 1.0f, 1.0f};
    car.axis = {0.0f, 1.0f, 0.0f};
    car.model = LoadModel("resources/models/Mach5.glb");
    car.rotation = 0.0f;
    car.health = 10;
    car.fuel = 1;
    car.consumption = 0.005;
    car.maxSpeed = 2.0;
    car.minSpeed = 0.5;
    car.velocity = 0.85;
    car.cameraPos = {0.0f, 0.4f, 0.1f};

    Camera camera = {car.cameraPos, (Vector3){0.0f, 0.4f, -1.0f}, (Vector3){0.0f, 1.0f, 0.0f}, 50.0f, 0};

    std::vector<Scenery> rightBuildings = MakeObject((Vector3){25, 0, 0}, (Vector3){10, 25, 10}, (Vector3){15, 60, 15}, DARKBLUE, 10);
    std::vector<Scenery> leftBuildings = MakeObject((Vector3){-25, 0, 0}, (Vector3){10, 25, 10}, (Vector3){15, 60, 15}, DARKBLUE, 10);

    std::vector<Scenery> roads = MakeObject((Vector3){0, 0.01, 10}, (Vector3){20, 0.1, 10}, (Vector3){20, 0.1, 10}, GRAY, 13);

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        // DisableCursor();
        // Update
        // std::string cStr = Vector3ToString(camera.position);
        // std::string pStr = Vector3ToString(player.position);
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
            car.velocity += Lerp(0.0, 0.1, 0.5);
        } else if (IsKeyDown(KEY_DOWN)) {
            car.velocity -= Lerp(0.0, 0.05, 0.5);
        }
        else if (car.velocity >= car.minSpeed){
            car.velocity -= Lerp(0.0, 0.01, 0.1);;
        }

        float radialPRotation = Utils::ConvertAngleToRadial(car.rotation);
        float amout = cosf(radialPRotation) * 0.3f;
        Clamp(amout, -0.3f, 0.3f);

        if (IsKeyDown(KEY_RIGHT) && car.position.x <= 10) {
            car.rotation -= 0.5;
            car.position.x += amout;
            camera.position.x += amout;
            camera.target.x += amout;
        } else if (IsKeyDown(KEY_LEFT) && car.position.x >= -10) {
            car.rotation += 0.5;
            car.position.x -= amout;
            camera.position.x -= amout;
            camera.target.x -= amout;
        } else {
            car.rotation = Lerp(car.rotation, 0.0f, 0.07f);
        }

        // else if (IsKeyDown(KEY_UP)) camera.position.y += 0.01f;
        // else if (IsKeyDown(KEY_DOWN)) camera.position.y -= 0.01f;
        // else if (IsKeyDown(KEY_A)) camera.position.x -= 0.01f;
        // else if (IsKeyDown(KEY_D)) camera.position.x += 0.01f;
        // else if (IsKeyDown(KEY_S)) camera.position.z -= 0.01f;
        // else if (IsKeyDown(KEY_W)) camera.position.z += 0.01f;

        // Draw
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModelEx(car.model, car.position, car.axis, car.rotation, car.size, WHITE);
        DrawCube((Vector3){0.0, 0.0, 0.0}, 100, 0.0, 100, DARKGREEN);

        for (size_t i = 0; i < roads.size(); i++) {
            roads[i].pos.z += car.velocity;
            if (i % 2 == 0) {
                DrawCube(roads[i].pos, roads[i].size.x, roads[i].size.y, roads[i].size.z, GRAY);
            } else {
                DrawCube(roads[i].pos, roads[i].size.x, roads[i].size.y, roads[i].size.z, LIGHTGRAY);
            }

            if (roads[i].pos.z >= 20) {
                roads[i].pos.z = -100.0f;
            }
        }

        for (size_t i = 0; i < rightBuildings.size(); i++) {
            rightBuildings[i].pos.z += car.velocity;
            if (i % 2 == 0) {
                DrawCube(rightBuildings[i].pos, rightBuildings[i].size.x, rightBuildings[i].size.y, rightBuildings[i].size.z, DARKGRAY);
            } else {
                DrawCube(rightBuildings[i].pos, rightBuildings[i].size.x, rightBuildings[i].size.y, rightBuildings[i].size.z, DARKBLUE);
            }

            if (rightBuildings[i].pos.z >= 20) {
                rightBuildings[i].pos.z = -100.0f;
            }
        }

        for (size_t i = 0; i < leftBuildings.size(); i++) {
            leftBuildings[i].pos.z += car.velocity;
            if (i % 2 == 0) {
                DrawCube(leftBuildings[i].pos, leftBuildings[i].size.x, leftBuildings[i].size.y, leftBuildings[i].size.z, DARKGRAY);
            } else {
                DrawCube(leftBuildings[i].pos, leftBuildings[i].size.x, leftBuildings[i].size.y, leftBuildings[i].size.z, DARKBLUE);
            }

            if (leftBuildings[i].pos.z >= 20) {
                leftBuildings[i].pos.z = -100.0f;
            }
        }

        // DrawGrid(100, 1.0f);

        EndMode3D();

        if (car.velocity < 0.1f || car.health <= 0.0f) {
            DrawText("Ta seco", 550, 360, 50, VIOLET);
        }

        DrawText("NICE GRAPHICS", 10, 30, 10, BLACK);
        DrawText(std::to_string(car.velocity).c_str(), 220, 80, 20, BLACK);
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
