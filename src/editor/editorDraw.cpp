#include "editorDraw.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

#include <raylib.h>

#include "uiText.hpp"
#include <raymath.h>

#include "debug/debugIcons.hpp"
#include "debug/levelDebugDraw.hpp"
#include "debug/ui/debugUi.hpp"
#include "game/gameWorld.hpp"
#include "gameplay/props.hpp"
#include "gameplay/smoking/smokingConfig.hpp"
#include "entity/entityRegistry.hpp"
#include "gameplay/worldActions.hpp"
#include "interactionSystem.hpp"
#include "level/levelData.hpp"
#include "level/levelLoader.hpp"
#include "level/levelRuntimeConfig.hpp"

namespace {

std::string DisplayNameFromPath(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    std::string filename = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = filename.find_last_of('.');
    return dot == std::string::npos ? filename : filename.substr(0, dot);
}

Quaternion QuaternionFromEulerDegrees(Vector3 degrees) {
    return QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
}

Vector3 EulerDegreesFromQuaternion(Quaternion rotation) {
    Vector3 radians = QuaternionToEuler(rotation);
    return Vector3{radians.x * RAD2DEG, radians.y * RAD2DEG, radians.z * RAD2DEG};
}

// ─── Widget helpers ──────────────────────────────────────────────────────────

bool DebugMenuItem(Rectangle rect, const char* text, bool enabled = true, bool checked = false) {
    Vector2 mouse = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovered ? Color{70, 70, 78, 255} : Color{42, 42, 48, 245});
    DrawRectangleLinesEx(rect, 1.0f, Color{78, 78, 86, 255});
    if (checked) {
        DrawUiText("✓", static_cast<int>(rect.x + 8), static_cast<int>(rect.y + 5), 18, RAYWHITE);
    }
    DrawUiText(text, static_cast<int>(rect.x + 28), static_cast<int>(rect.y + 6), 18, enabled ? RAYWHITE : GRAY);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool DebugButton(Rectangle rect, const char* text) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    DrawRectangleRec(rect, hovered ? Color{78, 78, 88, 255} : Color{56, 56, 64, 255});
    DrawRectangleLinesEx(rect, 1.0f, Color{95, 95, 105, 255});
    int textWidth = MeasureUiText(text, 16);
    DrawUiText(text,
        static_cast<int>(rect.x + rect.width * 0.5f - textWidth * 0.5f),
        static_cast<int>(rect.y + 6), 16, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool DebugFloatSlider(Rectangle rect, const char* label, float& value, float minValue, float maxValue) {
    Vector2 mouse = GetMousePosition();
    Rectangle bar = Rectangle{rect.x + 150.0f, rect.y + 8.0f, rect.width - 230.0f, 8.0f};
    bool changed = false;
    if (CheckCollisionPointRec(mouse, Rectangle{bar.x, rect.y, bar.width, rect.height})
        && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        float t = Clamp((mouse.x - bar.x) / bar.width, 0.0f, 1.0f);
        value = minValue + (maxValue - minValue) * t;
        changed = true;
    }
    float normalized = Clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
    DrawUiText(label, static_cast<int>(rect.x), static_cast<int>(rect.y + 2), 16, RAYWHITE);
    DrawRectangleRec(bar, Color{55, 55, 62, 255});
    DrawRectangleRec(Rectangle{bar.x, bar.y, bar.width * normalized, bar.height}, Color{90, 130, 210, 255});
    DrawCircle(static_cast<int>(bar.x + bar.width * normalized),
        static_cast<int>(bar.y + bar.height * 0.5f), 6.0f, RAYWHITE);
    DrawUiText(TextFormat("%.3f", value),
        static_cast<int>(rect.x + rect.width - 70.0f), static_cast<int>(rect.y + 2), 16, LIGHTGRAY);
    return changed;
}

bool DebugTextInput(Rectangle rect, const char* label, std::string& text, int fieldId, DebugUiState& ui) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.activeTextField = fieldId;
    }
    bool active = ui.activeTextField == fieldId;
    bool changed = false;
    DrawUiText(label, static_cast<int>(rect.x), static_cast<int>(rect.y + 5), 16, RAYWHITE);
    Rectangle inputRect = Rectangle{rect.x + 24.0f, rect.y, rect.width - 24.0f, rect.height};
    DrawRectangleRec(inputRect, active ? Color{46, 56, 72, 255} : Color{36, 36, 42, 255});
    DrawRectangleLinesEx(inputRect, 1.0f,
        active ? Color{120, 150, 220, 255} : Color{85, 85, 95, 255});
    DrawUiText(text.c_str(),
        static_cast<int>(inputRect.x + 6.0f), static_cast<int>(inputRect.y + 5.0f), 16, RAYWHITE);
    if (active) {
        int key = GetCharPressed();
        while (key > 0) {
            char c = static_cast<char>(key);
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
                if (text.size() < 20) {
                    text.push_back(c);
                    changed = true;
                }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) {
            text.pop_back();
            changed = true;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            ui.activeTextField = 0;
        }
    }
    return changed;
}

void DrawVector3Value(Vector2 pos, const char* label, const Vector3& value) {
    DrawUiText(TextFormat("%s: %.2f %.2f %.2f", label, value.x, value.y, value.z),
        static_cast<int>(pos.x), static_cast<int>(pos.y), 16, LIGHTGRAY);
}

bool BeginDebugPanel(DebugUiState& ui, int panelId, Vector2& position, bool& pinned,
    const char* title, float width, float height) {
    Vector2 mouse = GetMousePosition();
    Rectangle panelRect = Rectangle{position.x, position.y, width, height};
    Rectangle titleRect = Rectangle{position.x, position.y, width, 28.0f};
    Rectangle pinRect = Rectangle{position.x + width - 30.0f, position.y + 4.0f, 22.0f, 20.0f};

    if (ui.enabled && CheckCollisionPointRec(mouse, pinRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        pinned = !pinned;
    }
    if (ui.enabled && CheckCollisionPointRec(mouse, titleRect)
        && !CheckCollisionPointRec(mouse, pinRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.draggingPanel = panelId;
        ui.dragOffset = Vector2Subtract(mouse, position);
    }
    if (ui.draggingPanel == panelId) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            position = Vector2Subtract(mouse, ui.dragOffset);
            position.x = Clamp(position.x, 0.0f, static_cast<float>(GetScreenWidth()) - width);
            position.y = Clamp(position.y, 0.0f, static_cast<float>(GetScreenHeight()) - 28.0f);
        } else {
            ui.draggingPanel = -1;
        }
    }
    DrawRectangleRec(panelRect, Color{28, 28, 32, 220});
    DrawRectangleRec(titleRect, Color{42, 42, 48, 245});
    DrawRectangleLinesEx(panelRect, 1.0f, Color{80, 80, 88, 255});
    DrawUiText(title, static_cast<int>(position.x + 12.0f), static_cast<int>(position.y + 6.0f), 18, RAYWHITE);
    DrawRectangleRec(pinRect, pinned ? Color{80, 120, 80, 255} : Color{65, 65, 72, 255});
    DrawRectangleLinesEx(pinRect, 1.0f, Color{95, 95, 105, 255});
    DrawUiText("P", static_cast<int>(pinRect.x + 6.0f), static_cast<int>(pinRect.y + 2.0f), 16, RAYWHITE);
    return true;
}

// ─── Vector input helpers ─────────────────────────────────────────────────────

void SetVectorInput(std::string inputs[3], Vector3 value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.x); inputs[0] = buffer;
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.y); inputs[1] = buffer;
    std::snprintf(buffer, sizeof(buffer), "%.3f", value.z); inputs[2] = buffer;
}

bool ParseVectorInput(const std::string inputs[3], Vector3& value) {
    char* end = nullptr;
    value.x = std::strtof(inputs[0].c_str(), &end);
    if (end == inputs[0].c_str()) return false;
    value.y = std::strtof(inputs[1].c_str(), &end);
    if (end == inputs[1].c_str()) return false;
    value.z = std::strtof(inputs[2].c_str(), &end);
    return end != inputs[2].c_str();
}

void SetLevelNodeInputs(DebugUiState& ui, const LevelDebugNode& node) {
    SetVectorInput(ui.levelPositionInput, node.position);
    SetVectorInput(ui.levelRotationInput, EulerDegreesFromQuaternion(node.rotation));
    SetVectorInput(ui.levelScaleInput, node.scale);
}

// ─── Selection ID helpers ─────────────────────────────────────────────────────

int CharacterSelectionId(int index) { return -1000 - index; }
int CharacterIndexFromSelection(int selection) { return -1000 - selection; }
int CharacterPartSelectionId(int characterIndex, int kind, int partIndex) {
    return -200000 - characterIndex * 10000 - kind * 1000 - partIndex;
}
int LightSelectionId(int index) { return -300000 - index; }
int LightIndexFromSelection(int selection) { return -300000 - selection; }
int PropSelectionId(int index) { return -400000 - index; }
int PropIndexFromSelection(int selection) { return -400000 - selection; }

bool DecodeCharacterPartSelection(int selection, int& characterIndex, int& kind, int& partIndex) {
    if (selection > -200000) return false;
    int value = -200000 - selection;
    characterIndex = value / 10000;
    value %= 10000;
    kind = value / 1000;
    partIndex = value % 1000;
    return true;
}

// ─── Light / scene helpers ────────────────────────────────────────────────────

const char* LightTypeName(LightType type) {
    switch (type) {
        case LightType::Directional: return "Directional";
        case LightType::Point:       return "Point";
        case LightType::Spot:        return "Spot";
    }
    return "Directional";
}

Vector3 CameraForward(const Camera& camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    return Vector3LengthSqr(forward) > 0.0001f ? forward : Vector3{0.0f, 0.0f, -1.0f};
}

Vector3 DirectionToEulerDegrees(Vector3 direction) {
    direction = Vector3Normalize(direction);
    float yaw = atan2f(direction.x, -direction.z);
    float pitch = asinf(Clamp(direction.y, -1.0f, 1.0f));
    return Vector3{pitch * RAD2DEG, yaw * RAD2DEG, 0.0f};
}

std::string BuildNewLightId(const LightingConfig& lighting, const char* prefix) {
    return TextFormat("%s_%02d", prefix, static_cast<int>(lighting.lights.size()) + 1);
}

void AddDebugLight(GameWorld& gameWorld, LightType type) {
    Vector3 forward = CameraForward(gameWorld.render.camera);
    LevelLightConfig light;
    light.type = type;
    light.position = Vector3Add(gameWorld.render.camera.position,
        Vector3Scale(forward, 6.0f));
    light.rotation = QuaternionFromEulerDegrees(DirectionToEulerDegrees(forward));
    light.color = Color{255, 220, 170, 255};
    light.intensity = 1.0f;
    light.castShadows = false;
    if (type == LightType::Directional) {
        light.id = BuildNewLightId(gameWorld.world.runtimeConfig.lighting, "dir");
    } else if (type == LightType::Point) {
        light.id = BuildNewLightId(gameWorld.world.runtimeConfig.lighting, "point");
        light.range = 8.0f;
    } else {
        light.id = BuildNewLightId(gameWorld.world.runtimeConfig.lighting, "spot");
        light.range = 10.0f;
        light.innerConeDegrees = 18.0f;
        light.outerConeDegrees = 32.0f;
    }
    gameWorld.world.runtimeConfig.lighting.lights.push_back(light);
    int index = static_cast<int>(gameWorld.world.runtimeConfig.lighting.lights.size()) - 1;
    gameWorld.debugUi.selectedLevelNode = LightSelectionId(index);
    SetVectorInput(gameWorld.debugUi.levelPositionInput, light.position);
    SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(light.rotation));
    gameWorld.debugUi.levelSidebarOpen = true;
    gameWorld.debugUi.configDirty = true;
}

void AddDebugProp(GameWorld& gameWorld, const PropLibraryItem& item) {
    Vector3 forward = CameraForward(gameWorld.render.camera);
    LevelPropConfig prop;
    prop.id = TextFormat("prop_%02d", static_cast<int>(gameWorld.world.runtimeConfig.props.size()) + 1);
    prop.modelPath = item.path;
    prop.position = Vector3Add(gameWorld.render.camera.position, Vector3Scale(forward, 6.0f));
    prop.rotation = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
    prop.scale = Vector3{1.0f, 1.0f, 1.0f};
    gameWorld.world.runtimeConfig.props.push_back(prop);
    LoadRuntimeProps(gameWorld.world.props, gameWorld.world.runtimeConfig.props, gameWorld.render.fogShader);
    int index = static_cast<int>(gameWorld.world.runtimeConfig.props.size()) - 1;
    gameWorld.debugUi.selectedLevelNode = PropSelectionId(index);
    SetVectorInput(gameWorld.debugUi.levelPositionInput, prop.position);
    SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(prop.rotation));
    SetVectorInput(gameWorld.debugUi.levelScaleInput, prop.scale);
    gameWorld.debugUi.levelSidebarOpen = true;
    gameWorld.debugUi.configDirty = true;
}

void DeleteSelectedDebugRoot(GameWorld& gameWorld) {
    if (gameWorld.debugUi.selectedLevelNode <= -400000) {
        int propIndex = PropIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (propIndex < 0 || propIndex >= static_cast<int>(gameWorld.world.props.size())) return;
        gameWorld.world.runtimeConfig.props.erase(gameWorld.world.runtimeConfig.props.begin() + propIndex);
        UnloadRuntimeProps(gameWorld.world.props);
        LoadRuntimeProps(gameWorld.world.props, gameWorld.world.runtimeConfig.props, gameWorld.render.fogShader);
        gameWorld.debugUi.selectedLevelNode = -1;
        gameWorld.debugUi.configDirty = true;
        return;
    }
    if (gameWorld.debugUi.selectedLevelNode <= -300000) {
        int lightIndex = LightIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (lightIndex < 0 || lightIndex >= static_cast<int>(
            gameWorld.world.runtimeConfig.lighting.lights.size())) return;
        gameWorld.world.runtimeConfig.lighting.lights.erase(
            gameWorld.world.runtimeConfig.lighting.lights.begin() + lightIndex);
        gameWorld.debugUi.selectedLevelNode = -1;
        gameWorld.debugUi.configDirty = true;
        return;
    }
    if (gameWorld.debugUi.selectedLevelNode <= -1000
        && gameWorld.debugUi.selectedLevelNode > -200000) {
        int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (characterIndex < 0 || characterIndex >= static_cast<int>(
            gameWorld.world.runtimeConfig.characters.size())) return;
        gameWorld.world.runtimeConfig.characters.erase(
            gameWorld.world.runtimeConfig.characters.begin() + characterIndex);
        DestroyEntityRegistry(gameWorld.npcs);
        gameWorld.npcs = LoadEntityRegistry(gameWorld.world.runtimeConfig.characters);
        ApplyRuntimeRenderConfig(gameWorld);
        gameWorld.debugUi.selectedLevelNode = -1;
        gameWorld.debugUi.configDirty = true;
    }
}

// ─── Transform helpers ────────────────────────────────────────────────────────

void ApplySelectedLevelNodeInputs(GameWorld& gameWorld) {
    if (gameWorld.debugUi.selectedLevelNode < 0
        || gameWorld.debugUi.selectedLevelNode >= static_cast<int>(gameWorld.world.level.debugNodes.size()))
        return;
    Vector3 position = Vector3Zero();
    Vector3 rotationDegrees = Vector3Zero();
    Vector3 scale = Vector3{1.0f, 1.0f, 1.0f};
    if (!ParseVectorInput(gameWorld.debugUi.levelPositionInput, position)) return;
    if (!ParseVectorInput(gameWorld.debugUi.levelRotationInput, rotationDegrees)) return;
    if (!ParseVectorInput(gameWorld.debugUi.levelScaleInput, scale)) return;
    ApplyLevelDebugNodeTransform(gameWorld.world.level, gameWorld.debugUi.selectedLevelNode,
        position, QuaternionFromEulerDegrees(rotationDegrees), scale);
    ReloadJoltLevelPhysics(gameWorld);
}

bool GetSelectedTransform(GameWorld& gameWorld, Vector3& position, Quaternion& rotation,
    Vector3& scale, bool& saveToLevelConfig) {
    saveToLevelConfig = false;
    scale = Vector3{1.0f, 1.0f, 1.0f};
    if (gameWorld.debugUi.selectedLevelNode == -2) {
        position = gameWorld.world.level.rootPosition;
        rotation = gameWorld.world.level.rootRotation;
        return true;
    }
    if (gameWorld.debugUi.selectedLevelNode <= -1000
        && gameWorld.debugUi.selectedLevelNode > -200000) {
        int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (characterIndex < 0 || characterIndex >= static_cast<int>(
            gameWorld.npcs.characters.size())) return false;
        position = gameWorld.npcs.characters[characterIndex].rootPosition;
        rotation = gameWorld.npcs.characters[characterIndex].rootRotation;
        saveToLevelConfig = true;
        return true;
    }
    if (gameWorld.debugUi.selectedLevelNode <= -400000) {
        int propIndex = PropIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (propIndex < 0 || propIndex >= static_cast<int>(gameWorld.world.props.size())) return false;
        const RuntimeProp& prop = gameWorld.world.props[propIndex];
        position = prop.config.position;
        rotation = prop.config.rotation;
        scale = prop.config.scale;
        saveToLevelConfig = true;
        return true;
    }
    if (gameWorld.debugUi.selectedLevelNode <= -300000) {
        int lightIndex = LightIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (lightIndex < 0 || lightIndex >= static_cast<int>(
            gameWorld.world.runtimeConfig.lighting.lights.size())) return false;
        const LevelLightConfig& light = gameWorld.world.runtimeConfig.lighting.lights[lightIndex];
        position = light.position;
        rotation = light.rotation;
        saveToLevelConfig = true;
        return true;
    }
    if (gameWorld.debugUi.selectedLevelNode >= 0
        && gameWorld.debugUi.selectedLevelNode < static_cast<int>(gameWorld.world.level.debugNodes.size())) {
        const LevelDebugNode& node = gameWorld.world.level.debugNodes[gameWorld.debugUi.selectedLevelNode];
        position = node.position;
        rotation = node.rotation;
        scale = node.scale;
        return node.kind != LevelDebugNodeKind::Visual;
    }
    return false;
}

void SetSelectedTransform(GameWorld& gameWorld, Vector3 position, Quaternion rotation, Vector3 scale) {
    if (gameWorld.debugUi.selectedLevelNode == -2) {
        ApplyLevelRootTransform(gameWorld.world.level, position, rotation);
        ReloadJoltLevelPhysics(gameWorld);
    } else if (gameWorld.debugUi.selectedLevelNode <= -1000
        && gameWorld.debugUi.selectedLevelNode > -200000) {
        int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (characterIndex >= 0 && characterIndex < static_cast<int>(
            gameWorld.npcs.characters.size())) {
            ApplyCharacterRootTransform(
                gameWorld.npcs.characters[characterIndex], position, rotation);
            gameWorld.debugUi.configDirty = true;
        }
    } else if (gameWorld.debugUi.selectedLevelNode <= -400000) {
        int propIndex = PropIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (propIndex >= 0 && propIndex < static_cast<int>(gameWorld.world.props.size())) {
            RuntimeProp& prop = gameWorld.world.props[propIndex];
            prop.config.position = position;
            prop.config.rotation = rotation;
            prop.config.scale = scale;
            gameWorld.world.runtimeConfig.props[propIndex] = prop.config;
            gameWorld.debugUi.configDirty = true;
        }
    } else if (gameWorld.debugUi.selectedLevelNode <= -300000) {
        int lightIndex = LightIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (lightIndex >= 0 && lightIndex < static_cast<int>(
            gameWorld.world.runtimeConfig.lighting.lights.size())) {
            LevelLightConfig& light = gameWorld.world.runtimeConfig.lighting.lights[lightIndex];
            light.position = position;
            light.rotation = rotation;
            gameWorld.debugUi.configDirty = true;
        }
    } else if (gameWorld.debugUi.selectedLevelNode >= 0
        && gameWorld.debugUi.selectedLevelNode < static_cast<int>(gameWorld.world.level.debugNodes.size())) {
        ApplyLevelDebugNodeTransform(
            gameWorld.world.level, gameWorld.debugUi.selectedLevelNode, position, rotation, scale);
        ReloadJoltLevelPhysics(gameWorld);
    }
    SetVectorInput(gameWorld.debugUi.levelPositionInput, position);
    SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(rotation));
    SetVectorInput(gameWorld.debugUi.levelScaleInput, scale);
}

// ─── Teleport helpers ─────────────────────────────────────────────────────────

void TeleportDebugCamera(GameWorld& gameWorld, Vector3 position) {
    Vector3 forward = Vector3Normalize(
        Vector3Subtract(gameWorld.render.camera.target, gameWorld.render.camera.position));
    if (Vector3LengthSqr(forward) <= 0.0001f) forward = Vector3{0.0f, 0.0f, -1.0f};
    gameWorld.render.camera.position = position;
    gameWorld.render.camera.target = Vector3Add(position, forward);
    SyncDebugCameraRotation(gameWorld.render.debugCamera, gameWorld.render.camera);
}

void TeleportPerson(GameWorld& gameWorld, Vector3 position) {
    gameWorld.player.state.position = position;
    gameWorld.player.state.velocity = Vector3Zero();
    gameWorld.player.state.grounded = false;
    if (gameWorld.player.physics) {
        gameWorld.player.physics->SetCharacterPosition(position);
        gameWorld.player.physics->SetCharacterVelocity(Vector3Zero());
    }
    gameWorld.player.physicsAccumulator = 0.0f;
    ApplyPersonCamera(gameWorld.render.camera, gameWorld.player.state, gameWorld.player.config, 1.0f);
}

// ─── Transform gizmo ─────────────────────────────────────────────────────────

void DrawTransformGizmo(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled
        || !gameWorld.debugUi.levelSidebarOpen
        || gameWorld.debugUi.transformGizmoMode == 0) return;

    Vector3 position = Vector3Zero();
    Quaternion rotation = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 scale = Vector3{1.0f, 1.0f, 1.0f};
    bool saveToConfig = false;
    if (!GetSelectedTransform(gameWorld, position, rotation, scale, saveToConfig)) return;

    const Vector3 axes[3] = {
        Vector3{1.0f, 0.0f, 0.0f},
        Vector3{0.0f, 1.0f, 0.0f},
        Vector3{0.0f, 0.0f, 1.0f}
    };
    const Color colors[3] = {RED, GREEN, BLUE};
    Vector2 mouse = GetMousePosition();

    float bestDistance = 99999.0f;
    int hoveredAxis = -1;
    for (int i = 0; i < 3; ++i) {
        Vector3 axis = gameWorld.debugUi.transformGizmoMode == 2
            ? Vector3RotateByQuaternion(axes[i], rotation) : axes[i];
        Vector3 end = Vector3Add(position, Vector3Scale(axis, 1.0f));
        DrawLine3D(position, end, colors[i]);
        DrawSphere(end, 0.08f, colors[i]);
        Vector2 screenEnd = GetWorldToScreen(end, gameWorld.render.camera);
        float distance = Vector2Distance(mouse, screenEnd);
        if (distance < bestDistance && distance < 18.0f) {
            bestDistance = distance;
            hoveredAxis = i;
        }
    }

    if (!gameWorld.debugUi.draggingTransformGizmo
        && hoveredAxis >= 0 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        gameWorld.debugUi.draggingTransformGizmo = true;
        gameWorld.debugUi.transformGizmoAxis = hoveredAxis;
        gameWorld.debugUi.transformGizmoLastMouse = mouse;
    }
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        gameWorld.debugUi.draggingTransformGizmo = false;
        gameWorld.debugUi.transformGizmoAxis = -1;
    }
    if (!gameWorld.debugUi.draggingTransformGizmo
        || gameWorld.debugUi.transformGizmoAxis < 0) return;

    Vector2 delta = Vector2Subtract(mouse, gameWorld.debugUi.transformGizmoLastMouse);
    gameWorld.debugUi.transformGizmoLastMouse = mouse;
    int axisIndex = gameWorld.debugUi.transformGizmoAxis;
    Vector3 axis = gameWorld.debugUi.transformGizmoMode == 2
        ? Vector3RotateByQuaternion(axes[axisIndex], rotation) : axes[axisIndex];
    Vector2 screenPos = GetWorldToScreen(position, gameWorld.render.camera);
    Vector2 screenAxis = Vector2Subtract(
        GetWorldToScreen(Vector3Add(position, axis), gameWorld.render.camera), screenPos);
    float screenAxisLen = Vector2Length(screenAxis);
    if (screenAxisLen <= 0.001f) return;
    screenAxis = Vector2Scale(screenAxis, 1.0f / screenAxisLen);
    float mouseAlongAxis = Vector2DotProduct(delta, screenAxis);

    if (gameWorld.debugUi.transformGizmoMode == 1) {
        position = Vector3Add(position, Vector3Scale(axis, mouseAlongAxis * 0.025f));
    } else if (gameWorld.debugUi.transformGizmoMode == 2) {
        Quaternion deltaRotation = QuaternionFromAxisAngle(axis, mouseAlongAxis * 0.01f);
        rotation = QuaternionNormalize(QuaternionMultiply(deltaRotation, rotation));
    } else if (gameWorld.debugUi.transformGizmoMode == 3) {
        float deltaScale = mouseAlongAxis * 0.01f;
        if (axisIndex == 0) scale.x = std::max(0.01f, scale.x + deltaScale);
        if (axisIndex == 1) scale.y = std::max(0.01f, scale.y + deltaScale);
        if (axisIndex == 2) scale.z = std::max(0.01f, scale.z + deltaScale);
    }
    SetSelectedTransform(gameWorld, position, rotation, scale);
}

// ─── Level sidebar ────────────────────────────────────────────────────────────

bool LevelNodeMatchesTab(const LevelDebugNode& node, int tab) {
    if (tab == 1) return node.kind == LevelDebugNodeKind::Visual;
    if (tab == 2) return node.kind == LevelDebugNodeKind::Collider;
    if (tab == 3) return node.kind == LevelDebugNodeKind::Icon;
    if (tab == 4) return node.kind == LevelDebugNodeKind::Spawn
        || node.kind == LevelDebugNodeKind::Trigger;
    if (tab == 5) return node.kind == LevelDebugNodeKind::Invisible
        || node.kind == LevelDebugNodeKind::Other;
    return false;
}

void DrawLevelSidebar(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled || !gameWorld.debugUi.levelSidebarOpen) return;

    float width = 360.0f;
    float x = static_cast<float>(GetScreenWidth()) - width;
    float h = static_cast<float>(GetScreenHeight()) - 34.0f;
    Rectangle panel = Rectangle{x, 34.0f, width, h};
    DrawRectangleRec(panel, Color{24, 24, 30, 235});
    DrawRectangleLinesEx(panel, 1.0f, Color{80, 80, 88, 255});
    gameWorld.debugUi.levelSidebarTab = 0;
    DrawUiText("Inspector", static_cast<int>(x + 14.0f), 44, 20, RAYWHITE);

    const char* tabs[] = {"ROOT"};
    for (int i = 0; i < 1; ++i) {
        Rectangle tab = Rectangle{x + 12.0f + i * 56.0f, 74.0f, 52.0f, 26.0f};
        bool active = gameWorld.debugUi.levelSidebarTab == i;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), tab);
        DrawRectangleRec(tab,
            active ? Color{78, 92, 120, 255} : hovered ? Color{64, 64, 72, 255} : Color{42, 42, 48, 255});
        DrawRectangleLinesEx(tab, 1.0f, Color{85, 85, 95, 255});
        DrawUiText(tabs[i], static_cast<int>(tab.x + 5.0f), static_cast<int>(tab.y + 6.0f), 13, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.levelSidebarTab = i;
            gameWorld.debugUi.levelSidebarScroll = 0;
        }
    }

    if (gameWorld.debugUi.levelSidebarTab == 0) {
        float rootY = 110.0f;
        Rectangle rootRow = Rectangle{x + 12.0f, rootY, width - 24.0f, 24.0f};
        bool selected = gameWorld.debugUi.selectedLevelNode == -2;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rootRow);
        DrawRectangleRec(rootRow,
            selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
        std::string levelName = DisplayNameFromPath(gameWorld.world.level.name);
        DrawUiText(TextFormat("LEVEL %s", levelName.c_str()),
            static_cast<int>(rootRow.x + 6.0f), static_cast<int>(rootRow.y + 5.0f), 14, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.selectedLevelNode = -2;
            SetVectorInput(gameWorld.debugUi.levelPositionInput, gameWorld.world.level.rootPosition);
            SetVectorInput(gameWorld.debugUi.levelRotationInput,
                EulerDegreesFromQuaternion(gameWorld.world.level.rootRotation));
        }
        rootY += 28.0f;
        for (std::size_t c = 0; c < gameWorld.npcs.characters.size(); ++c) {
            const InteractableCharacter& character = gameWorld.npcs.characters[c];
            Rectangle row = Rectangle{x + 12.0f, rootY, width - 24.0f, 24.0f};
            int selectionId = CharacterSelectionId(static_cast<int>(c));
            selected = gameWorld.debugUi.selectedLevelNode == selectionId;
            hovered = CheckCollisionPointRec(GetMousePosition(), row);
            DrawRectangleRec(row,
                selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
            std::string characterName = DisplayNameFromPath(character.modelPath);
            DrawUiText(TextFormat("CHAR %s", characterName.c_str()),
                static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 5.0f), 14, RAYWHITE);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                gameWorld.debugUi.selectedLevelNode = selectionId;
                SetVectorInput(gameWorld.debugUi.levelPositionInput, character.rootPosition);
                SetVectorInput(gameWorld.debugUi.levelRotationInput,
                    EulerDegreesFromQuaternion(character.rootRotation));
            }
            rootY += 28.0f;
        }
        for (std::size_t p = 0; p < gameWorld.world.props.size(); ++p) {
            const RuntimeProp& prop = gameWorld.world.props[p];
            Rectangle row = Rectangle{x + 12.0f, rootY, width - 24.0f, 24.0f};
            int selectionId = PropSelectionId(static_cast<int>(p));
            selected = gameWorld.debugUi.selectedLevelNode == selectionId;
            hovered = CheckCollisionPointRec(GetMousePosition(), row);
            DrawRectangleRec(row,
                selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
            DrawUiText(TextFormat("PROP %s", prop.config.id.c_str()),
                static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 5.0f), 14, RAYWHITE);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                gameWorld.debugUi.selectedLevelNode = selectionId;
                SetVectorInput(gameWorld.debugUi.levelPositionInput, prop.config.position);
                SetVectorInput(gameWorld.debugUi.levelRotationInput, EulerDegreesFromQuaternion(prop.config.rotation));
                SetVectorInput(gameWorld.debugUi.levelScaleInput, prop.config.scale);
            }
            rootY += 28.0f;
        }
        for (std::size_t l = 0; l < gameWorld.world.runtimeConfig.lighting.lights.size(); ++l) {
            const LevelLightConfig& light = gameWorld.world.runtimeConfig.lighting.lights[l];
            Rectangle row = Rectangle{x + 12.0f, rootY, width - 24.0f, 24.0f};
            int selectionId = LightSelectionId(static_cast<int>(l));
            selected = gameWorld.debugUi.selectedLevelNode == selectionId;
            hovered = CheckCollisionPointRec(GetMousePosition(), row);
            DrawRectangleRec(row,
                selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
            DrawUiText(TextFormat("LIGHT %s [%s]", light.id.c_str(), LightTypeName(light.type)),
                static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 5.0f), 14, RAYWHITE);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                gameWorld.debugUi.selectedLevelNode = selectionId;
                SetVectorInput(gameWorld.debugUi.levelPositionInput, light.position);
                SetVectorInput(gameWorld.debugUi.levelRotationInput,
                    EulerDegreesFromQuaternion(light.rotation));
            }
            rootY += 28.0f;
        }
    }

    int matchingCount = 0;
    for (std::size_t i = 0; i < gameWorld.world.level.debugNodes.size(); ++i) {
        if (LevelNodeMatchesTab(gameWorld.world.level.debugNodes[i], gameWorld.debugUi.levelSidebarTab))
            ++matchingCount;
    }
    for (std::size_t c = 0; c < gameWorld.npcs.characters.size(); ++c) {
        const InteractableCharacter& character = gameWorld.npcs.characters[c];
        if (gameWorld.debugUi.levelSidebarTab == 1)
            matchingCount += static_cast<int>(character.visualParts.size());
        if (gameWorld.debugUi.levelSidebarTab == 2)
            matchingCount += static_cast<int>(character.colliders.size());
        if (gameWorld.debugUi.levelSidebarTab == 3)
            matchingCount += static_cast<int>(character.iconParts.size());
    }

    float listY = 110.0f;
    float rowH = 22.0f;
    int visibleRows = 14;
    if (CheckCollisionPointRec(GetMousePosition(),
        Rectangle{x, listY, width, visibleRows * rowH})) {
        gameWorld.debugUi.levelSidebarScroll -= static_cast<int>(GetMouseWheelMove());
        gameWorld.debugUi.levelSidebarScroll = Clamp(
            gameWorld.debugUi.levelSidebarScroll, 0, std::max(0, matchingCount - visibleRows));
    }

    int skipped = 0;
    int drawn = 0;
    for (std::size_t i = 0; i < gameWorld.world.level.debugNodes.size() && drawn < visibleRows; ++i) {
        const LevelDebugNode& node = gameWorld.world.level.debugNodes[i];
        if (!LevelNodeMatchesTab(node, gameWorld.debugUi.levelSidebarTab)) continue;
        if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
        Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
        bool selected = gameWorld.debugUi.selectedLevelNode == static_cast<int>(i);
        bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
        DrawRectangleRec(row,
            selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255} : Color{34, 34, 40, 255});
        std::string nodeName = DisplayNameFromPath(node.name);
        DrawUiText(TextFormat("LEVEL %s", nodeName.c_str()),
            static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            gameWorld.debugUi.selectedLevelNode = static_cast<int>(i);
            SetLevelNodeInputs(gameWorld.debugUi, node);
        }
        ++drawn;
    }
    for (std::size_t c = 0; c < gameWorld.npcs.characters.size() && drawn < visibleRows; ++c) {
        const InteractableCharacter& character = gameWorld.npcs.characters[c];
        if (gameWorld.debugUi.levelSidebarTab == 1) {
            for (std::size_t p = 0; p < character.visualParts.size() && drawn < visibleRows; ++p) {
                if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
                int selectionId = CharacterPartSelectionId(
                    static_cast<int>(c), 0, static_cast<int>(p));
                Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
                bool selected = gameWorld.debugUi.selectedLevelNode == selectionId;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
                DrawRectangleRec(row,
                    selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255}
                             : Color{34, 34, 40, 255});
                std::string characterName = DisplayNameFromPath(character.modelPath);
                DrawUiText(TextFormat("CHAR %s/%s",
                    characterName.c_str(), character.visualParts[p].name.c_str()),
                    static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    gameWorld.debugUi.selectedLevelNode = selectionId;
                ++drawn;
            }
        } else if (gameWorld.debugUi.levelSidebarTab == 2) {
            for (std::size_t p = 0; p < character.colliders.size() && drawn < visibleRows; ++p) {
                if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
                int selectionId = CharacterPartSelectionId(
                    static_cast<int>(c), 1, static_cast<int>(p));
                Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
                bool selected = gameWorld.debugUi.selectedLevelNode == selectionId;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
                DrawRectangleRec(row,
                    selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255}
                             : Color{34, 34, 40, 255});
                std::string characterName = DisplayNameFromPath(character.modelPath);
                DrawUiText(TextFormat("CHAR %s/%s",
                    characterName.c_str(), character.colliders[p].name.c_str()),
                    static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    gameWorld.debugUi.selectedLevelNode = selectionId;
                ++drawn;
            }
        } else if (gameWorld.debugUi.levelSidebarTab == 3) {
            for (std::size_t p = 0; p < character.iconParts.size() && drawn < visibleRows; ++p) {
                if (skipped < gameWorld.debugUi.levelSidebarScroll) { ++skipped; continue; }
                int selectionId = CharacterPartSelectionId(
                    static_cast<int>(c), 2, static_cast<int>(p));
                Rectangle row = Rectangle{x + 12.0f, listY + drawn * rowH, width - 24.0f, rowH};
                bool selected = gameWorld.debugUi.selectedLevelNode == selectionId;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
                DrawRectangleRec(row,
                    selected ? Color{80, 90, 120, 255} : hovered ? Color{54, 54, 62, 255}
                             : Color{34, 34, 40, 255});
                std::string characterName = DisplayNameFromPath(character.modelPath);
                DrawUiText(TextFormat("CHAR %s/%s",
                    characterName.c_str(), character.iconParts[p].name.c_str()),
                    static_cast<int>(row.x + 6.0f), static_cast<int>(row.y + 4.0f), 14, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    gameWorld.debugUi.selectedLevelNode = selectionId;
                ++drawn;
            }
        }
    }
    DrawUiText(TextFormat("%d items", matchingCount),
        static_cast<int>(x + 14.0f), static_cast<int>(listY + visibleRows * rowH + 4.0f), 14, GRAY);

    float editY = listY + visibleRows * rowH + 24.0f;
    if (gameWorld.debugUi.selectedLevelNode == -2
        || (gameWorld.debugUi.selectedLevelNode <= -1000
            && gameWorld.debugUi.selectedLevelNode > -200000)
        || gameWorld.debugUi.selectedLevelNode <= -400000) {
        bool editingCharacter = gameWorld.debugUi.selectedLevelNode <= -1000
            && gameWorld.debugUi.selectedLevelNode > -200000;
        bool editingProp = gameWorld.debugUi.selectedLevelNode <= -400000;
        DrawUiText(editingProp ? "PROP ROOT" : editingCharacter ? "CHARACTER ROOT" : "LEVEL ROOT",
            static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
        DrawUiText("Move/rotate/scale root inteiro.",
            static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE); editY += 26.0f;

        bool changed = false;
        DrawUiText("Position", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelPositionInput[0], 401, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelPositionInput[1], 402, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelPositionInput[2], 403, gameWorld.debugUi) || changed;
        editY += 34.0f;

        DrawUiText("Rotation", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelRotationInput[0], 404, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelRotationInput[1], 405, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelRotationInput[2], 406, gameWorld.debugUi) || changed;
        editY += 34.0f;
        if (DebugButton(Rectangle{x + 14.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 1 ? "Move ON" : "Move"))
            gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 1 ? 0 : 1;
        if (DebugButton(Rectangle{x + 120.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 2 ? "Rotate ON" : "Rotate"))
            gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 2 ? 0 : 2;
        if (DebugButton(Rectangle{x + 226.0f, editY, 96.0f, 24.0f}, gameWorld.debugUi.transformGizmoMode == 3 ? "Scale ON" : "Scale"))
            gameWorld.debugUi.transformGizmoMode = gameWorld.debugUi.transformGizmoMode == 3 ? 0 : 3;
        editY += 34.0f;

        if (changed) {
            Vector3 position = Vector3Zero();
            Vector3 rotationDegrees = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.levelPositionInput, position)
                && ParseVectorInput(gameWorld.debugUi.levelRotationInput, rotationDegrees)) {
                if (editingProp) {
                    Vector3 scale = Vector3{1.0f, 1.0f, 1.0f};
                    ParseVectorInput(gameWorld.debugUi.levelScaleInput, scale);
                    SetSelectedTransform(gameWorld, position, QuaternionFromEulerDegrees(rotationDegrees), scale);
                } else if (editingCharacter) {
                    int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
                    if (characterIndex >= 0 && characterIndex < static_cast<int>(
                        gameWorld.npcs.characters.size())) {
                        ApplyCharacterRootTransform(
                            gameWorld.npcs.characters[characterIndex],
                            position, QuaternionFromEulerDegrees(rotationDegrees));
                        gameWorld.debugUi.configDirty = true;
                    }
                } else {
                    ApplyLevelRootTransform(
                        gameWorld.world.level, position, QuaternionFromEulerDegrees(rotationDegrees));
                    ReloadJoltLevelPhysics(gameWorld);
                }
            }
        }
        if (DebugButton(Rectangle{x + 14.0f, editY, 190.0f, 26.0f}, "Teleport to Camera")) {
            if (editingCharacter) {
                int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
                if (characterIndex >= 0 && characterIndex < static_cast<int>(
                    gameWorld.npcs.characters.size())) {
                    ApplyCharacterRootTransform(
                        gameWorld.npcs.characters[characterIndex],
                        gameWorld.render.camera.position,
                        gameWorld.npcs.characters[characterIndex].rootRotation);
                    gameWorld.debugUi.configDirty = true;
                    SetVectorInput(gameWorld.debugUi.levelPositionInput,
                        gameWorld.npcs.characters[characterIndex].rootPosition);
                }
            } else {
                ApplyLevelRootTransform(
                    gameWorld.world.level, gameWorld.render.camera.position, gameWorld.world.level.rootRotation);
                SetVectorInput(gameWorld.debugUi.levelPositionInput, gameWorld.world.level.rootPosition);
                ReloadJoltLevelPhysics(gameWorld);
            }
        }
        if (editingCharacter) {
            editY += 34.0f;
            Rectangle deleteRect = Rectangle{x + 214.0f, editY, 92.0f, 26.0f};
            DrawRectangleRec(deleteRect,
                CheckCollisionPointRec(GetMousePosition(), deleteRect)
                    ? Color{150, 55, 45, 255} : Color{115, 42, 36, 255});
            DrawRectangleLinesEx(deleteRect, 1.0f, Color{190, 90, 75, 255});
            DrawDebugIcon2D(gameWorld.render.debugIcons, "delete",
                Vector2{deleteRect.x + 8.0f, deleteRect.y + 2.0f}, 22.0f, ORANGE);
            DrawUiText("Delete",
                static_cast<int>(deleteRect.x + 34.0f), static_cast<int>(deleteRect.y + 6.0f),
                14, RAYWHITE);
            if (CheckCollisionPointRec(GetMousePosition(), deleteRect)
                && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                DeleteSelectedDebugRoot(gameWorld);
                return;
            }
            editY += 40.0f;
            const char* detailTabs[] = {"VISUAL", "COL", "ICON", "SP/TR", "OTHER"};
            for (int i = 0; i < 5; ++i) {
                Rectangle tab = Rectangle{x + 12.0f + i * 66.0f, editY, 62.0f, 24.0f};
                bool active = gameWorld.debugUi.inspectorDetailTab == i;
                bool hovered = CheckCollisionPointRec(GetMousePosition(), tab);
                DrawRectangleRec(tab,
                    active ? Color{78, 92, 120, 255} : hovered ? Color{64, 64, 72, 255}
                           : Color{42, 42, 48, 255});
                DrawUiText(detailTabs[i],
                    static_cast<int>(tab.x + 4.0f), static_cast<int>(tab.y + 6.0f), 12, RAYWHITE);
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    gameWorld.debugUi.inspectorDetailTab = i;
            }
            editY += 30.0f;
            int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
            if (characterIndex >= 0 && characterIndex < static_cast<int>(
                gameWorld.npcs.characters.size())) {
                const InteractableCharacter& character =
                    gameWorld.npcs.characters[characterIndex];
                if (gameWorld.debugUi.inspectorDetailTab == 0) {
                    for (std::size_t i = 0; i < character.visualParts.size() && editY < h - 24.0f;
                         ++i, editY += 20.0f)
                        DrawUiText(character.visualParts[i].name.c_str(),
                            static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, LIGHTGRAY);
                } else if (gameWorld.debugUi.inspectorDetailTab == 1) {
                    for (std::size_t i = 0; i < character.colliders.size() && editY < h - 24.0f;
                         ++i, editY += 20.0f)
                        DrawUiText(character.colliders[i].name.c_str(),
                            static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, LIGHTGRAY);
                } else if (gameWorld.debugUi.inspectorDetailTab == 2) {
                    for (std::size_t i = 0; i < character.iconParts.size() && editY < h - 24.0f;
                         ++i, editY += 20.0f)
                        DrawUiText(character.iconParts[i].name.c_str(),
                            static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, LIGHTGRAY);
                } else {
                    DrawUiText("Sem dados nesta aba para CHAR.",
                        static_cast<int>(x + 18.0f), static_cast<int>(editY), 13, GRAY);
                }
            }
        }
    }

    if (gameWorld.debugUi.selectedLevelNode <= -300000) {
        int lightIndex = LightIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (lightIndex >= 0 && lightIndex < static_cast<int>(
            gameWorld.world.runtimeConfig.lighting.lights.size())) {
            LevelLightConfig& light =
                gameWorld.world.runtimeConfig.lighting.lights[lightIndex];
            DrawUiText(TextFormat("LIGHT %s [%s]", light.id.c_str(), LightTypeName(light.type)),
                static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
            DrawUiText("Directional usa rotation: forward local -Z.",
                static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE); editY += 26.0f;

            bool changed = false;
            if (DebugButton(Rectangle{x + 14.0f, editY, 120.0f, 24.0f},
                light.enabled ? "Enabled ON" : "Enabled OFF")) {
                light.enabled = !light.enabled;
                changed = true;
            }
            if (DebugButton(Rectangle{x + 144.0f, editY, 120.0f, 24.0f},
                light.castShadows ? "Shadows ON" : "Shadows OFF")) {
                light.castShadows = !light.castShadows;
                changed = true;
            }
            editY += 34.0f;

            DrawUiText("Position", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
            changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelPositionInput[0], 501, gameWorld.debugUi) || changed;
            changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelPositionInput[1], 502, gameWorld.debugUi) || changed;
            changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelPositionInput[2], 503, gameWorld.debugUi) || changed;
            editY += 34.0f;

            DrawUiText("Rotation", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
            changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelRotationInput[0], 504, gameWorld.debugUi) || changed;
            changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelRotationInput[1], 505, gameWorld.debugUi) || changed;
            changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelRotationInput[2], 506, gameWorld.debugUi) || changed;
            editY += 34.0f;

            if (DebugButton(Rectangle{x + 14.0f, editY, 96.0f, 24.0f},
                gameWorld.debugUi.transformGizmoMode == 1 ? "Move ON" : "Move"))
                gameWorld.debugUi.transformGizmoMode =
                    gameWorld.debugUi.transformGizmoMode == 1 ? 0 : 1;
            if (DebugButton(Rectangle{x + 120.0f, editY, 96.0f, 24.0f},
                gameWorld.debugUi.transformGizmoMode == 2 ? "Rotate ON" : "Rotate"))
                gameWorld.debugUi.transformGizmoMode =
                    gameWorld.debugUi.transformGizmoMode == 2 ? 0 : 2;
            editY += 34.0f;

            float red = static_cast<float>(light.color.r);
            float green = static_cast<float>(light.color.g);
            float blue = static_cast<float>(light.color.b);
            changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Red", red, 0.0f, 255.0f) || changed; editY += 28.0f;
            changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Green", green, 0.0f, 255.0f) || changed; editY += 28.0f;
            changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Blue", blue, 0.0f, 255.0f) || changed; editY += 28.0f;
            changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Intensity", light.intensity, 0.0f, 4.0f) || changed; editY += 28.0f;
            if (light.type == LightType::Point || light.type == LightType::Spot)
                changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Range", light.range, 0.5f, 50.0f) || changed;
            editY += 28.0f;
            if (light.type == LightType::Spot) {
                changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Inner", light.innerConeDegrees, 1.0f, 80.0f) || changed; editY += 28.0f;
                changed = DebugFloatSlider(Rectangle{x + 14.0f, editY, 320.0f, 24.0f}, "Outer", light.outerConeDegrees, 2.0f, 90.0f) || changed; editY += 28.0f;
                if (light.outerConeDegrees < light.innerConeDegrees + 1.0f)
                    light.outerConeDegrees = light.innerConeDegrees + 1.0f;
            }
            editY += 6.0f;
            light.color = Color{
                static_cast<unsigned char>(Clamp(red, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(green, 0.0f, 255.0f)),
                static_cast<unsigned char>(Clamp(blue, 0.0f, 255.0f)),
                255};

            if (changed) {
                gameWorld.debugUi.configDirty = true;
                Vector3 position = Vector3Zero();
                Vector3 rotationDegrees = Vector3Zero();
                if (ParseVectorInput(gameWorld.debugUi.levelPositionInput, position)
                    && ParseVectorInput(gameWorld.debugUi.levelRotationInput, rotationDegrees)) {
                    light.position = position;
                    light.rotation = QuaternionFromEulerDegrees(rotationDegrees);
                }
            }
            if (DebugButton(Rectangle{x + 14.0f, editY, 190.0f, 26.0f}, "Teleport to Camera")) {
                light.position = gameWorld.render.camera.position;
                SetVectorInput(gameWorld.debugUi.levelPositionInput, light.position);
                gameWorld.debugUi.configDirty = true;
            }
            editY += 34.0f;
            Rectangle deleteRect = Rectangle{x + 214.0f, editY, 92.0f, 26.0f};
            DrawRectangleRec(deleteRect,
                CheckCollisionPointRec(GetMousePosition(), deleteRect)
                    ? Color{150, 55, 45, 255} : Color{115, 42, 36, 255});
            DrawRectangleLinesEx(deleteRect, 1.0f, Color{190, 90, 75, 255});
            DrawDebugIcon2D(gameWorld.render.debugIcons, "delete",
                Vector2{deleteRect.x + 8.0f, deleteRect.y + 2.0f}, 22.0f, ORANGE);
            DrawUiText("Delete",
                static_cast<int>(deleteRect.x + 34.0f), static_cast<int>(deleteRect.y + 6.0f),
                14, RAYWHITE);
            if (CheckCollisionPointRec(GetMousePosition(), deleteRect)
                && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                DeleteSelectedDebugRoot(gameWorld);
                return;
            }
        }
    }

    int selectedCharacterIndex = 0;
    int selectedCharacterPartKind = 0;
    int selectedCharacterPartIndex = 0;
    if (DecodeCharacterPartSelection(gameWorld.debugUi.selectedLevelNode,
        selectedCharacterIndex, selectedCharacterPartKind, selectedCharacterPartIndex)) {
        if (selectedCharacterIndex >= 0 && selectedCharacterIndex < static_cast<int>(
            gameWorld.npcs.characters.size())) {
            const InteractableCharacter& character =
                gameWorld.npcs.characters[selectedCharacterIndex];
            const char* kindName = selectedCharacterPartKind == 0 ? "VISUAL"
                : selectedCharacterPartKind == 1 ? "COL" : "ICON";
            const char* partName = "";
            if (selectedCharacterPartKind == 0
                && selectedCharacterPartIndex < static_cast<int>(character.visualParts.size()))
                partName = character.visualParts[selectedCharacterPartIndex].name.c_str();
            if (selectedCharacterPartKind == 1
                && selectedCharacterPartIndex < static_cast<int>(character.colliders.size()))
                partName = character.colliders[selectedCharacterPartIndex].name.c_str();
            if (selectedCharacterPartKind == 2
                && selectedCharacterPartIndex < static_cast<int>(character.iconParts.size()))
                partName = character.iconParts[selectedCharacterPartIndex].name.c_str();
            std::string characterName = DisplayNameFromPath(character.modelPath);
            DrawUiText(TextFormat("CHAR %s", characterName.c_str()),
                static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
            DrawUiText(TextFormat("%s %s", kindName, partName),
                static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, LIGHTGRAY); editY += 22.0f;
            DrawUiText("Part read-only. Use ROOT/CHAR to move whole GLB.",
                static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE);
        }
    }

    if (gameWorld.debugUi.selectedLevelNode >= 0
        && gameWorld.debugUi.selectedLevelNode < static_cast<int>(gameWorld.world.level.debugNodes.size())) {
        const LevelDebugNode& node = gameWorld.world.level.debugNodes[gameWorld.debugUi.selectedLevelNode];
        std::string nodeName = DisplayNameFromPath(node.name);
        DrawUiText(nodeName.c_str(),
            static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, YELLOW); editY += 24.0f;
        DrawUiText(LevelDebugNodeKindName(node.kind),
            static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, GRAY); editY += 24.0f;
        if (node.kind == LevelDebugNodeKind::Visual) {
            DrawUiText("VISUAL read-only: level render voltou para DrawModel().",
                static_cast<int>(x + 14.0f), static_cast<int>(editY), 14, ORANGE);
            editY += 22.0f;
        }

        DrawUiText("Position", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        bool changed = false;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelPositionInput[0], 301, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelPositionInput[1], 302, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelPositionInput[2], 303, gameWorld.debugUi) || changed;
        editY += 34.0f;

        DrawUiText("Rotation", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelRotationInput[0], 304, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelRotationInput[1], 305, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelRotationInput[2], 306, gameWorld.debugUi) || changed;
        editY += 34.0f;
        if (DebugButton(Rectangle{x + 14.0f, editY, 96.0f, 24.0f},
            gameWorld.debugUi.transformGizmoMode == 1 ? "Move ON" : "Move"))
            gameWorld.debugUi.transformGizmoMode =
                gameWorld.debugUi.transformGizmoMode == 1 ? 0 : 1;
        if (DebugButton(Rectangle{x + 120.0f, editY, 96.0f, 24.0f},
            gameWorld.debugUi.transformGizmoMode == 2 ? "Rotate ON" : "Rotate"))
            gameWorld.debugUi.transformGizmoMode =
                gameWorld.debugUi.transformGizmoMode == 2 ? 0 : 2;
        editY += 34.0f;

        DrawUiText("Scale", static_cast<int>(x + 14.0f), static_cast<int>(editY), 16, RAYWHITE); editY += 22.0f;
        changed = DebugTextInput(Rectangle{x + 14.0f, editY, 100.0f, 24.0f}, "X", gameWorld.debugUi.levelScaleInput[0], 307, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 124.0f, editY, 100.0f, 24.0f}, "Y", gameWorld.debugUi.levelScaleInput[1], 308, gameWorld.debugUi) || changed;
        changed = DebugTextInput(Rectangle{x + 234.0f, editY, 100.0f, 24.0f}, "Z", gameWorld.debugUi.levelScaleInput[2], 309, gameWorld.debugUi) || changed;
        editY += 38.0f;

        if (changed && node.kind != LevelDebugNodeKind::Visual)
            ApplySelectedLevelNodeInputs(gameWorld);
        if (node.kind != LevelDebugNodeKind::Visual
            && DebugButton(Rectangle{x + 14.0f, editY, 190.0f, 26.0f}, "Teleport to Camera")) {
            TeleportLevelDebugNodeToCamera(
                gameWorld.world.level, gameWorld.debugUi.selectedLevelNode, gameWorld.render.camera);
            SetLevelNodeInputs(gameWorld.debugUi,
                gameWorld.world.level.debugNodes[gameWorld.debugUi.selectedLevelNode]);
            ReloadJoltLevelPhysics(gameWorld);
        }
    }
}

// ─── Debug panels ─────────────────────────────────────────────────────────────

void DrawDebugPanels(GameWorld& gameWorld) {
    bool showPersonStatus = gameWorld.debugUi.showPersonStatus
        && (gameWorld.debugUi.enabled || gameWorld.debugUi.pinPersonStatus);
    bool showPhysicsPanel = gameWorld.debugUi.showPhysicsPanel
        && (gameWorld.debugUi.enabled || gameWorld.debugUi.pinPhysicsPanel);

    if (showPersonStatus) {
        Vector2& pos = gameWorld.debugUi.personStatusPos;
        BeginDebugPanel(gameWorld.debugUi, 0, pos, gameWorld.debugUi.pinPersonStatus,
            "Person Status", 280.0f, 132.0f);
        float speed = Vector3Length(
            Vector3{gameWorld.player.state.velocity.x, 0.0f, gameWorld.player.state.velocity.z});
        DrawUiText(TextFormat("Speed: %.2f m/s", speed),
            static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 44), 18, RAYWHITE);
        DrawUiText(gameWorld.player.state.grounded ? "Grounded" : "Airborne",
            static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 68),
            18, gameWorld.player.state.grounded ? GREEN : RED);
        DrawUiText(TextFormat("Pos: %.1f %.1f %.1f",
            gameWorld.player.state.position.x, gameWorld.player.state.position.y,
            gameWorld.player.state.position.z),
            static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 92), 18, RAYWHITE);
    }

    if (showPhysicsPanel) {
        Vector2& pos = gameWorld.debugUi.physicsPanelPos;
        BeginDebugPanel(gameWorld.debugUi, 2, pos, gameWorld.debugUi.pinPhysicsPanel,
            "Physics Panel", 320.0f, 110.0f);
        DrawUiText("Physics debug tools coming soon",
            static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 50), 18, LIGHTGRAY);
    }

    if (gameWorld.debugUi.debugTeleportOpen) {
        Vector2& pos = gameWorld.debugUi.debugTeleportPos;
        bool pinned = false;
        BeginDebugPanel(gameWorld.debugUi, 3, pos, pinned,
            "Debug Camera Teleport", 320.0f, 150.0f);
        DrawUiText(TextFormat("Camera: %.2f %.2f %.2f",
            gameWorld.render.camera.position.x, gameWorld.render.camera.position.y,
            gameWorld.render.camera.position.z),
            static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 38), 16, LIGHTGRAY);
        DebugTextInput(Rectangle{pos.x + 14.0f, pos.y + 64.0f, 88.0f, 26.0f}, "X",
            gameWorld.debugUi.debugTeleportInput[0], 101, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 112.0f, pos.y + 64.0f, 88.0f, 26.0f}, "Y",
            gameWorld.debugUi.debugTeleportInput[1], 102, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 210.0f, pos.y + 64.0f, 88.0f, 26.0f}, "Z",
            gameWorld.debugUi.debugTeleportInput[2], 103, gameWorld.debugUi);
        if (DebugButton(Rectangle{pos.x + 14.0f, pos.y + 108.0f, 140.0f, 26.0f}, "Teleport")) {
            Vector3 target = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.debugTeleportInput, target))
                TeleportDebugCamera(gameWorld, target);
        }
        if (DebugButton(Rectangle{pos.x + 164.0f, pos.y + 108.0f, 70.0f, 26.0f}, "Use cam")) {
            SetVectorInput(gameWorld.debugUi.debugTeleportInput, gameWorld.render.camera.position);
        }
        if (DebugButton(Rectangle{pos.x + 244.0f, pos.y + 108.0f, 60.0f, 26.0f}, "Close")) {
            gameWorld.debugUi.debugTeleportOpen = false;
            gameWorld.debugUi.activeTextField = 0;
        }
    }

    if (gameWorld.debugUi.gameTeleportOpen) {
        Vector2& pos = gameWorld.debugUi.gameTeleportPos;
        bool pinned = false;
        BeginDebugPanel(gameWorld.debugUi, 4, pos, pinned, "Game Teleport", 320.0f, 140.0f);
        DrawUiText(TextFormat("Person: %.2f %.2f %.2f",
            gameWorld.player.state.position.x, gameWorld.player.state.position.y,
            gameWorld.player.state.position.z),
            static_cast<int>(pos.x + 14), static_cast<int>(pos.y + 38), 16, LIGHTGRAY);
        DebugTextInput(Rectangle{pos.x + 14.0f, pos.y + 64.0f, 88.0f, 26.0f}, "X",
            gameWorld.debugUi.gameTeleportInput[0], 201, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 112.0f, pos.y + 64.0f, 88.0f, 26.0f}, "Y",
            gameWorld.debugUi.gameTeleportInput[1], 202, gameWorld.debugUi);
        DebugTextInput(Rectangle{pos.x + 210.0f, pos.y + 64.0f, 88.0f, 26.0f}, "Z",
            gameWorld.debugUi.gameTeleportInput[2], 203, gameWorld.debugUi);
        if (DebugButton(Rectangle{pos.x + 14.0f, pos.y + 102.0f, 130.0f, 26.0f}, "Teleport Person")) {
            Vector3 target = Vector3Zero();
            if (ParseVectorInput(gameWorld.debugUi.gameTeleportInput, target))
                TeleportPerson(gameWorld, target);
        }
        if (DebugButton(Rectangle{pos.x + 286.0f, pos.y + 102.0f, 26.0f, 26.0f}, "X")) {
            gameWorld.debugUi.gameTeleportOpen = false;
            gameWorld.debugUi.activeTextField = 0;
        }
    }
}

void DrawDebugAddLightContextMenu(GameWorld& gameWorld) {
    static bool open = false;
    static Vector2 pos = Vector2Zero();
    static int side = 0;
    float sidebarWidth = gameWorld.debugUi.levelSidebarOpen || gameWorld.debugUi.levelConfigOpen ? 390.0f : 0.0f;
    Rectangle topBar = Rectangle{0.0f, 0.0f, static_cast<float>(GetScreenWidth()), 34.0f};
    Rectangle rightPanel = Rectangle{static_cast<float>(GetScreenWidth()) - sidebarWidth, 34.0f, sidebarWidth, static_cast<float>(GetScreenHeight()) - 34.0f};
    Vector2 mouse = GetMousePosition();
    bool overUi = CheckCollisionPointRec(mouse, topBar) || (sidebarWidth > 0.0f && CheckCollisionPointRec(mouse, rightPanel));
    if (gameWorld.debugUi.enabled && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_A) && !overUi) {
        open = true;
        pos = mouse;
        side = 0;
    }
    if (!open) return;

    Rectangle menu = Rectangle{pos.x, pos.y, 160.0f, 88.0f};
    DrawRectangleRec(menu, Color{32, 32, 38, 245});
    DrawRectangleLinesEx(menu, 1.0f, Color{95, 95, 105, 255});
    DrawUiText("Add", static_cast<int>(menu.x + 10.0f), static_cast<int>(menu.y + 8.0f), 16, YELLOW);
    Rectangle lights = Rectangle{menu.x + 8.0f, menu.y + 32.0f, menu.width - 16.0f, 24.0f};
    Rectangle props = Rectangle{menu.x + 8.0f, menu.y + 58.0f, menu.width - 16.0f, 24.0f};
    if (CheckCollisionPointRec(mouse, lights)) side = 1;
    if (CheckCollisionPointRec(mouse, props)) side = 2;
    DebugMenuItem(lights, "Lights >");
    DebugMenuItem(props, "Props >");

    Rectangle sideRect = Rectangle{menu.x + menu.width + 4.0f, menu.y + 32.0f, 230.0f, side == 2 ? 220.0f : 88.0f};
    bool overSide = false;
    if (side == 1) {
        overSide = CheckCollisionPointRec(mouse, sideRect);
        if (DebugMenuItem(Rectangle{sideRect.x + 8.0f, sideRect.y + 4.0f, sideRect.width - 16.0f, 24.0f}, "Directional Light")) { AddDebugLight(gameWorld, LightType::Directional); open = false; }
        if (DebugMenuItem(Rectangle{sideRect.x + 8.0f, sideRect.y + 30.0f, sideRect.width - 16.0f, 24.0f}, "Point Light")) { AddDebugLight(gameWorld, LightType::Point); open = false; }
        if (DebugMenuItem(Rectangle{sideRect.x + 8.0f, sideRect.y + 56.0f, sideRect.width - 16.0f, 24.0f}, "Spot Light")) { AddDebugLight(gameWorld, LightType::Spot); open = false; }
    } else if (side == 2) {
        std::vector<PropLibraryItem> items = ScanPropLibrary();
        int count = std::min(7, static_cast<int>(items.size()));
        sideRect.height = 10.0f + std::max(1, count) * 26.0f;
        DrawRectangleRec(sideRect, Color{32, 32, 38, 245});
        DrawRectangleLinesEx(sideRect, 1.0f, Color{95, 95, 105, 255});
        overSide = CheckCollisionPointRec(mouse, sideRect);
        if (items.empty()) {
            DebugMenuItem(Rectangle{sideRect.x + 8.0f, sideRect.y + 6.0f, sideRect.width - 16.0f, 24.0f}, "(empty)", false);
        }
        for (int i = 0; i < count; ++i) {
            if (DebugMenuItem(Rectangle{sideRect.x + 8.0f, sideRect.y + 6.0f + i * 26.0f, sideRect.width - 16.0f, 24.0f}, items[i].name.c_str())) { AddDebugProp(gameWorld, items[i]); open = false; }
        }
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(mouse, menu) && !overSide) open = false;
}

void DrawDebugLightIcons2D(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.levelSidebarOpen) return;
    for (std::size_t i = 0;
         i < gameWorld.world.runtimeConfig.lighting.lights.size(); ++i) {
        const LevelLightConfig& light = gameWorld.world.runtimeConfig.lighting.lights[i];
        if (!light.enabled) continue;
        Color color = gameWorld.debugUi.selectedLevelNode == LightSelectionId(static_cast<int>(i))
            ? YELLOW : light.color;
        const char* icon = light.type == LightType::Directional ? "wb_sunny"
            : light.type == LightType::Point ? "lightbulb" : "flashlight_on";
        DrawDebugIconBillboard(gameWorld.render.debugIcons, gameWorld.render.camera, icon, light.position,
            gameWorld.debugUi.selectedLevelNode == LightSelectionId(static_cast<int>(i))
                ? 34.0f : 28.0f,
            color);
    }
}

void DrawSmokingEmitIcon2D(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled
        || !gameWorld.debugUi.personPanelOpen
        || gameWorld.debugUi.personPanelTab != 1) return;
    const ParticleEmitterConfig& pcfg = gameWorld.player.smokingConfig.postPuffParticles;
    Vector3 emitPos = Vector3{
        gameWorld.player.state.position.x + pcfg.emitOffsetX,
        gameWorld.player.state.position.y + pcfg.emitOffsetY,
        gameWorld.player.state.position.z + pcfg.emitOffsetZ
    };
    DrawDebugIconBillboard(gameWorld.render.debugIcons, gameWorld.render.camera,
        "smoking_rooms", emitPos, 28.0f, Color{200, 200, 100, 220});
}

void DrawSmokingEmitGizmo(GameWorld& gameWorld) {
    if (!gameWorld.debugUi.enabled
        || !gameWorld.debugUi.personPanelOpen
        || gameWorld.debugUi.personPanelTab != 1) return;

    ParticleEmitterConfig& pcfg = gameWorld.player.smokingConfig.postPuffParticles;
    Vector3 origin = gameWorld.player.state.position;
    Vector3 gizmoPos = Vector3{
        origin.x + pcfg.emitOffsetX,
        origin.y + pcfg.emitOffsetY,
        origin.z + pcfg.emitOffsetZ
    };

    const Vector3 axes[3]   = {{1,0,0}, {0,1,0}, {0,0,1}};
    const Color   colors[3] = {RED, GREEN, BLUE};
    Vector2 mouse = GetMousePosition();

    float bestDist  = 99999.0f;
    int   hoveredAxis = -1;
    for (int i = 0; i < 3; ++i) {
        Vector3 end = Vector3Add(gizmoPos, Vector3Scale(axes[i], 1.2f));
        DrawLine3D(gizmoPos, end, colors[i]);
        DrawSphere(end, 0.07f, colors[i]);
        float dist = Vector2Distance(mouse, GetWorldToScreen(end, gameWorld.render.camera));
        if (dist < bestDist && dist < 18.0f) { bestDist = dist; hoveredAxis = i; }
    }

    DebugUiState& ui = gameWorld.debugUi;
    if (!ui.smokingGizmoDragging && hoveredAxis >= 0 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        ui.smokingGizmoDragging  = true;
        ui.smokingGizmoAxis      = hoveredAxis;
        ui.smokingGizmoLastMouse = mouse;
    }
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        ui.smokingGizmoDragging = false;
        ui.smokingGizmoAxis     = -1;
    }
    if (!ui.smokingGizmoDragging || ui.smokingGizmoAxis < 0) return;

    Vector2 delta   = Vector2Subtract(mouse, ui.smokingGizmoLastMouse);
    ui.smokingGizmoLastMouse = mouse;
    int axisIndex   = ui.smokingGizmoAxis;
    Vector2 screenPos  = GetWorldToScreen(gizmoPos, gameWorld.render.camera);
    Vector2 screenAxis = Vector2Subtract(
        GetWorldToScreen(Vector3Add(gizmoPos, axes[axisIndex]), gameWorld.render.camera), screenPos);
    float axisLen = Vector2Length(screenAxis);
    if (axisLen <= 0.001f) return;
    screenAxis = Vector2Scale(screenAxis, 1.0f / axisLen);
    float move = Vector2DotProduct(delta, screenAxis) * 0.025f;

    float* offsets[3] = {&pcfg.emitOffsetX, &pcfg.emitOffsetY, &pcfg.emitOffsetZ};
    *offsets[axisIndex] += move;
    gameWorld.debugUi.configDirty = true;
}

}  // namespace

// ─── Public API ───────────────────────────────────────────────────────────────

void editor::Draw3DOverlays(GameWorld& gameWorld) {
    DrawSmokingEmitGizmo(gameWorld);
    if (!gameWorld.debugUi.levelSidebarOpen) {
        DrawTransformGizmo(gameWorld);
        return;
    }

    for (std::size_t i = 0;
         i < gameWorld.world.runtimeConfig.lighting.lights.size(); ++i) {
        const LevelLightConfig& light = gameWorld.world.runtimeConfig.lighting.lights[i];
        if (!light.enabled) continue;
        Color color = light.color;
        if (gameWorld.debugUi.selectedLevelNode == LightSelectionId(static_cast<int>(i)))
            color = YELLOW;
        Vector3 direction = Vector3Normalize(
            Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, light.rotation));
        if (light.type == LightType::Point) {
            DrawSphereWires(light.position, light.range, 12, 12, color);
        } else if (light.type == LightType::Spot) {
            DrawLine3D(light.position,
                Vector3Add(light.position, Vector3Scale(direction, light.range)), color);
            float radius = tanf(light.outerConeDegrees * DEG2RAD) * light.range;
            DrawCylinderWiresEx(light.position,
                Vector3Add(light.position, Vector3Scale(direction, light.range)),
                0.0f, radius, 16, color);
        } else {
            DrawLine3D(light.position,
                Vector3Add(light.position, Vector3Scale(direction, 2.5f)), color);
        }
    }

    if (gameWorld.debugUi.selectedLevelNode == -2) {
        Vector3 p = gameWorld.world.level.rootPosition;
        DrawLine3D(p, Vector3Add(p, Vector3{1.0f, 0.0f, 0.0f}), RED);
        DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 1.0f, 0.0f}), GREEN);
        DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 0.0f, 1.0f}), BLUE);
    } else if (gameWorld.debugUi.selectedLevelNode <= -1000
        && gameWorld.debugUi.selectedLevelNode > -200000) {
        int characterIndex = CharacterIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (characterIndex >= 0 && characterIndex < static_cast<int>(
            gameWorld.npcs.characters.size())) {
            DrawCharacterDebugSelection(gameWorld.npcs.characters[characterIndex]);
        }
    } else if (gameWorld.debugUi.selectedLevelNode <= -300000) {
        int lightIndex = LightIndexFromSelection(gameWorld.debugUi.selectedLevelNode);
        if (lightIndex >= 0 && lightIndex < static_cast<int>(
            gameWorld.world.runtimeConfig.lighting.lights.size())) {
            const LevelLightConfig& light =
                gameWorld.world.runtimeConfig.lighting.lights[lightIndex];
            Vector3 direction = Vector3Normalize(
                Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, light.rotation));
            DrawLine3D(light.position,
                Vector3Add(light.position, Vector3Scale(direction, 3.5f)), YELLOW);
        }
    } else if (gameWorld.debugUi.selectedLevelNode <= -200000) {
        int characterIndex = 0, kind = 0, partIndex = 0;
        if (DecodeCharacterPartSelection(
            gameWorld.debugUi.selectedLevelNode, characterIndex, kind, partIndex)
            && characterIndex >= 0 && characterIndex < static_cast<int>(
                gameWorld.npcs.characters.size())) {
            const InteractableCharacter& character =
                gameWorld.npcs.characters[characterIndex];
            if (kind == 1 && partIndex >= 0
                && partIndex < static_cast<int>(character.colliders.size())) {
                const CharacterCapsule& capsule = character.colliders[partIndex];
                DrawCapsuleWires(capsule.bottom, capsule.top, capsule.radius, 12, 6, ORANGE);
                Vector3 p = Vector3Scale(Vector3Add(capsule.bottom, capsule.top), 0.5f);
                DrawLine3D(p, Vector3Add(p, Vector3{1.0f, 0.0f, 0.0f}), RED);
                DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 1.0f, 0.0f}), GREEN);
                DrawLine3D(p, Vector3Add(p, Vector3{0.0f, 0.0f, 1.0f}), BLUE);
            } else {
                DrawCharacterDebugSelection(character);
            }
        }
    } else {
        DrawLevelDebugSelection(gameWorld.world.level, gameWorld.debugUi.selectedLevelNode,
            gameWorld.render.camera);
    }

    DrawTransformGizmo(gameWorld);
}

void editor::Draw2DOverlays(GameWorld& gameWorld) {
    DrawSmokingEmitIcon2D(gameWorld);
    DrawDebugLightIcons2D(gameWorld);
    debug_ui::DrawTopBar(gameWorld, debug_ui::TopBarActions{
        RestartLevel, ResetGameWorld, SaveCurrentLevelRuntimeConfig});
    DrawDebugPanels(gameWorld);
    DrawLevelSidebar(gameWorld);
    debug_ui::DrawLevelConfigSidebar(gameWorld, debug_ui::LevelConfigActions{
        SaveCurrentLevelRuntimeConfig,
        ReloadCurrentLevelForConfig,
        LoadConfiguredLevel,
        MoveLevelConfigEntry
    });
    DrawDebugAddLightContextMenu(gameWorld);
}
