#include <raylib.h>
#include <raymath.h>

#include <string>
#include <vector>

enum class SysState {
    EDITOR,
    MENU,
    PLAYING,
    PAUSED
};

SysState sysState = SysState::PLAYING;

const int SCRWIDTH = 1280;
const int SCRHEIGHT = 720;

struct Scenery {
    Vector3 pos;
    Vector3 size;
    Color color;
};

std::string Vector3ToString(const Vector3 &vec) {
    return "(" + std::to_string(vec.x) + ", " +
           std::to_string(vec.y) + ", " +
           std::to_string(vec.z) + ")";
}

float ConvertAngleToRadial(float angle) {
    float result = angle * 3.14f / 180;
    return result;
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

    Camera camera = {{-0.12f, 0.4f, 10.11f}, (Vector3){0.0f, 0.0f, 0.0f}, (Vector3){0.0f, 1.0f, 0.0f}, 50.0f, 0};
    Model model = LoadModel("resources/models/Mach5.glb");
    float velocity = 0.5f;

    std::vector<Scenery> rightBuildings = MakeObject((Vector3){25, 0, 0}, (Vector3){10, 25, 10}, (Vector3){15, 60, 15}, DARKBLUE, 10);
    std::vector<Scenery> leftBuildings = MakeObject((Vector3){-25, 0, 0}, (Vector3){10, 25, 10}, (Vector3){15, 60, 15}, DARKBLUE, 10);

    std::vector<Scenery> roads = MakeObject((Vector3){0, 0.01, 10}, (Vector3){20, 0.1, 10}, (Vector3){20, 0.1, 10}, GRAY, 13);

    Vector3 playerPosition = {0.0f, 0.2f, 10.0f};
    Vector3 playerSize = {2.0f, 2.0f, 2.0f};
    Vector3 pRotationAxis = {0.0f, 1.0f, 0.0f};
    float pRotation = 0.0f;

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        // Update
        // std::string cStr = Vector3ToString(camera.position);
        std::string pStr = Vector3ToString(playerPosition);
        DisableCursor();
        float radialPRotation = ConvertAngleToRadial(pRotation);
        float amout = cosf(radialPRotation) * 0.3f;
        Clamp(amout, -0.3f, 0.3f);

        if (IsKeyDown(KEY_UP) && velocity < 0.85f) {
            velocity += 0.008f;
        } else if (IsKeyDown(KEY_DOWN) && velocity > 0.2f) {
            velocity -= 0.006f;
        }

        // Move player
        if (IsKeyDown(KEY_RIGHT) && playerPosition.x <= 10) {
            pRotation -= 0.5;
            playerPosition.x += amout;
            camera.position.x += amout;
            camera.target.x += amout;
        } else if (IsKeyDown(KEY_LEFT) && playerPosition.x >= -10) {
            pRotation += 0.5;
            playerPosition.x -= amout;
            camera.position.x -= amout;
            camera.target.x -= amout;
        } else {
            pRotation = Lerp(pRotation, 0.0f, 0.05f);
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
        DrawModelEx(model, playerPosition, pRotationAxis, pRotation, (Vector3){1.0f, 1.0f, 1.0f}, WHITE);
        DrawCube((Vector3){0.0, 0.0, 0.0}, 100, 0.0, 100, DARKGREEN);

        for (size_t i = 0; i < roads.size(); i++) {
            roads[i].pos.z += velocity;
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
            rightBuildings[i].pos.z += velocity;
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
            leftBuildings[i].pos.z += velocity;
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

        DrawText("NICE GRAPHICS", 10, 30, 10, BLACK);
        DrawText(std::to_string(ConvertAngleToRadial(pRotation)).c_str(), 220, 80, 20, BLACK);
        DrawText(pStr.c_str(), 220, 100, 20, BLACK);

        DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
