#include "cameraDebug.hpp"

void ControlCamera(Camera camera) {
    if (IsKeyDown(KEY_E)) {
        camera.position.y += 0.1f;
    }
    else if (IsKeyDown(KEY_Q)) {
        camera.position.y -= 0.1f;
    }
    else if (IsKeyDown(KEY_A)) {
        camera.position.x -= 0.1f;
    }
    else if (IsKeyDown(KEY_D)) {
        camera.position.x += 0.1f;
    }
    else if (IsKeyDown(KEY_S)) {
        camera.position.z -= 0.1f;
    }
    else if (IsKeyDown(KEY_W)) {
        camera.position.z += 0.1f;
    }
}