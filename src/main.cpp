#include <raylib.h>
#include <vector>
#include "sysSettings.cpp"

SysState sysState = SysState::PLAYING;

struct Road
{
    Vector3 pos;
    Vector3 size;
};


int main()
{
    InitWindow(1280, 720, "SPEED");

    Camera camera = {{0.0f, 2.0f, 10.0f}, (Vector3){0.0f, 0.0f, 0.0f}, (Vector3){0.0f, 1.0f, 0.0f}, 50.0f, 0};
    Model model = LoadModel("resources/models/mach5.glb"); 
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
    roads.push_back({ {0, 0, -110}, {20, 0.1, 10}});

    Vector3 playerPosition = { 0.0f, 1.0f, 2.0f };
    Vector3 playerSize = { 0.6f, 0.8f, 1.5f };
    Color playerColor = GREEN;

    SetTargetFPS(60);
    while (!WindowShouldClose()) 
    {
        // Update
        // UpdateCamera(&camera, CAMERA_FREE);
        // Move player
        if (IsKeyDown(KEY_RIGHT)) playerPosition.x += 0.2f;
        else if (IsKeyDown(KEY_LEFT)) playerPosition.x -= 0.2f;
        
        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                // DrawCubeV(playerPosition, playerSize, playerColor);
                DrawModelEx(model, playerPosition, (Vector3){ 0.0f, 0.0f, 1.0f }, 0.0f, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);


                for (size_t i = 0; i < roads.size(); i++)
                {
                    roads[i].pos.z += velocity;
                    if (i % 2 == 0) {
                        DrawCube(roads[i].pos, roads[i].size.x, roads[i].size.y, roads[i].size.z, GRAY);
                    }
                    else {
                        DrawCube(roads[i].pos, roads[i].size.x, roads[i].size.y, roads[i].size.z, LIGHTGRAY);
                    }

                    if (roads[i].pos.z >= 20) {
                        roads[i].pos.z = -110.0f;
                    }
                }

                DrawGrid(10, 1.0f);        // Draw a grid

            EndMode3D();

            DrawText("NICE GRAPHICS", 220, 40, 20, BLACK);

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
