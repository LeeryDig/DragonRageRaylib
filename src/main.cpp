namespace ray
{
#include <raylib.h>
}

ray::Vector3 spherePos{0.0f, 0.0f, 0.0f};

int main()
{
    ray::InitWindow(1250, 720, "NICE GRAPHICS");

    ray::Camera camera = {0};
    camera.position = (ray::Vector3){0.0f, 10.0f, 10.0f};
    camera.target = (ray::Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (ray::Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = ray::CAMERA_PERSPECTIVE;

    ray::Model model = ray::LoadModel("resources/models/Mazda.glb");
    ray::Texture2D texture = ray::LoadTexture("resources/models/MazdaBodyDiff.png");
    ray::SetMaterialTexture(&model.materials[0], ray::MATERIAL_MAP_DIFFUSE, texture);
    ray::DisableCursor(); 
    while (!ray::WindowShouldClose())
    {
        ray::UpdateCamera(&camera, ray::CAMERA_FIRST_PERSON);

        ray::BeginDrawing();
        ray::ClearBackground(ray::RAYWHITE);
            ray::BeginMode3D(camera);
                ray::DrawText("NICE GRAPHICS", 625, 320, 30, ray::WHITE);
                ray::DrawModelEx(model, spherePos, (ray::Vector3){ 1.0f, 0.0f, 0.0f }, 0.0f, (ray::Vector3){ 0.1f, 0.1f, 0.1f }, ray::WHITE);
                ray::DrawGrid(10, 1.0f);
            ray::EndMode3D();
        ray::EndDrawing();
    }

    ray::UnloadTexture(texture);
    ray::UnloadModel(model);

    ray::CloseWindow();

    return 0;
}