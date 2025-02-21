#include <string>

#include <raylib.h>
#include <vector>
#include "sysSettings.cpp"
#include <math.h> 

SysState sysState = SysState::PLAYING;

struct Road
{
    Vector3 pos;
    Vector3 size;
};

std::string Vector3ToString(const Vector3& vec) {
    return "(" + std::to_string(vec.x) + ", " +
                 std::to_string(vec.y) + ", " +
                 std::to_string(vec.z) + ")";
}

float Lerp(float start, float end, float speed) {
    float result = start + speed*(end - start);
    return result;
}

float ConvertAngleToRadial(float angle) {
    float result = angle * 3.14f/180;
    return result;
}


int main() {
    InitWindow(1280, 720, "SPEED");

    Camera camera = {{-0.12f, 0.4f, 10.11f}, (Vector3){0.0f, 0.0f, 0.0f}, (Vector3){0.0f, 1.0f, 0.0f}, 50.0f, 0};
    Model model = LoadModel("resources/models/Mach5.glb"); 
    float velocity = 0.5f;

    std::vector<Road> roads;
    roads.push_back({ {0, 0, 10}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, 0}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -10}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -20}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -30}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -40}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -50}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -60}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -70}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -80}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -90}, {20, 0.1, 10}});
    roads.push_back({ {0, 0, -100}, {20, 0.1, 10}});

    Vector3 playerPosition = { 0.0f, 0.2f, 10.0f };
    Vector3 playerSize = { 2.0f, 2.0f, 2.0f };
    Vector3 pRotationAxis = { 0.0f, 1.0f, 0.0f };
    float pRotation = 0.0f;
    Color playerColor = GREEN;

    SetTargetFPS(60);
    while (!WindowShouldClose()) 
    {
        // Update
        // UpdateCamera(&camera, CAMERA_FREE);
        // std::string cStr = Vector3ToString(camera.position);
        // std::string pStr = Vector3ToString(playerPosition);
        DisableCursor();
        float radialPRotation = ConvertAngleToRadial(pRotation);

        if (IsKeyDown(KEY_UP) && velocity < 0.85f) {
            velocity += 0.008f;
        }
        else if (IsKeyDown(KEY_DOWN) && velocity > 0.2f) {
            velocity -= 0.006f;
        }

        // Move player
        if (IsKeyDown(KEY_RIGHT) && radialPRotation > -0.3f) {
            pRotation -= 0.5;
            float amout = cosf(radialPRotation) * 0.3f;
            playerPosition.x += amout;
            camera.position.x += amout;
            camera.target.x += amout;
        }
        else if (IsKeyDown(KEY_LEFT) && radialPRotation < 0.3f) {
            pRotation += 0.5;
            float amout = cosf(radialPRotation) * 0.3f;
            playerPosition.x -= amout;
            camera.position.x -= amout;
            camera.target.x -= amout;
        }
        else {
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
                DrawModelEx(model, playerPosition, pRotationAxis, pRotation, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);

                for (size_t i = 0; i < roads.size(); i++) {
                    roads[i].pos.z += velocity;
                    if (i % 2 == 0) {
                        DrawCube(roads[i].pos, roads[i].size.x, roads[i].size.y, roads[i].size.z, GRAY);
                    }
                    else {
                        DrawCube(roads[i].pos, roads[i].size.x, roads[i].size.y, roads[i].size.z, LIGHTGRAY);
                    }

                    if (roads[i].pos.z >= 20) {
                        roads[i].pos.z = -100.0f;
                    }
                }

                DrawGrid(100, 1.0f);

            EndMode3D();

            DrawText("NICE GRAPHICS", 10, 30, 10, BLACK);
            DrawText(std::to_string(ConvertAngleToRadial(pRotation)).c_str(), 220, 80, 20, BLACK);
            // DrawText(pStr.c_str(), 220, 100, 20, BLACK);

            DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
