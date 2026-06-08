#include "uiText.hpp"
#include "entityRegistry.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "raylib.h"
#include "raymath.h"

#include "interactionSystem.hpp"

namespace {

float DistancePointToRay(Vector3 point, Ray ray) {
    Vector3 toPoint = Vector3Subtract(point, ray.position);
    float t = std::max(0.0f, Vector3DotProduct(toPoint, ray.direction));
    Vector3 closest = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    return Vector3Distance(point, closest);
}

float DistanceRayToCapsuleApprox(Ray ray, const CharacterCapsule& capsule) {
    float best = std::min(DistancePointToRay(capsule.bottom, ray), DistancePointToRay(capsule.top, ray));
    for (int i = 1; i < 6; ++i) {
        float t = static_cast<float>(i) / 6.0f;
        Vector3 point = Vector3Lerp(capsule.bottom, capsule.top, t);
        best = std::min(best, DistancePointToRay(point, ray));
    }
    return best;
}

void WrapAndDrawUiText(const std::string& text, int x, int y, int fontSize, int maxWidth, Color color) {
    std::string line;
    std::string word;
    int lineY = y;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        char c = i < text.size() ? text[i] : ' ';
        if (c == ' ' || c == '\n' || i == text.size()) {
            std::string test = line.empty() ? word : line + " " + word;
            if (!line.empty() && MeasureUiText(test.c_str(), fontSize) > maxWidth) {
                DrawUiText(line.c_str(), x, lineY, fontSize, color);
                lineY += fontSize + 6;
                line = word;
            } else {
                line = test;
            }
            word.clear();
            if (c == '\n') {
                DrawUiText(line.c_str(), x, lineY, fontSize, color);
                lineY += fontSize + 6;
                line.clear();
            }
        } else {
            word.push_back(c);
        }
    }
    if (!line.empty()) DrawUiText(line.c_str(), x, lineY, fontSize, color);
}

}  // namespace

// ─── Lifecycle ───────────────────────────────────────────────────────────────

EntityRegistry CreateEntityRegistry() {
    EntityRegistry reg = {};
    reg.nextId = 1u;
    reg.focusedId = INVALID_ENTITY;
    reg.activeDialogueId = INVALID_ENTITY;
    reg.selectedChoiceIndex = 0;
    reg.dialogueOpen = false;
    return reg;
}

void DestroyEntityRegistry(EntityRegistry& registry) {
    for (std::size_t i = 0; i < registry.characters.size(); ++i) {
        if (registry.characters[i].hasModel) UnloadModel(registry.characters[i].model);
    }
    registry.characters.clear();
    registry.characterIds.clear();
}

EntityRegistry LoadEntityRegistry(const std::vector<CharacterSpawnConfig>& configs) {
    EntityRegistry registry = CreateEntityRegistry();
    for (std::size_t i = 0; i < configs.size(); ++i) {
        InteractableCharacter character;
        if (!LoadInteractableCharacter(configs[i].configPath, character)) continue;

        Quaternion rotation = QuaternionFromEuler(
            configs[i].rotationDegrees.x * DEG2RAD,
            configs[i].rotationDegrees.y * DEG2RAD,
            configs[i].rotationDegrees.z * DEG2RAD);
        ApplyCharacterRootTransform(character, configs[i].position, rotation);
        TraceLog(LOG_INFO,
            "Entity: loaded %s from %s at %.2f %.2f %.2f",
            character.id.c_str(),
            configs[i].configPath.c_str(),
            configs[i].position.x,
            configs[i].position.y,
            configs[i].position.z);
        AddCharacter(registry, character);
    }
    return registry;
}

// ─── CRUD ────────────────────────────────────────────────────────────────────

EntityId AddCharacter(EntityRegistry& registry, InteractableCharacter character) {
    EntityId id = registry.nextId++;
    registry.characterIds.push_back(id);
    registry.characters.push_back(character);
    return id;
}

void RemoveCharacter(EntityRegistry& registry, EntityId id) {
    for (std::size_t i = 0; i < registry.characterIds.size(); ++i) {
        if (registry.characterIds[i] != id) continue;
        if (registry.characters[i].hasModel) UnloadModel(registry.characters[i].model);
        registry.characterIds.erase(registry.characterIds.begin() + static_cast<std::ptrdiff_t>(i));
        registry.characters.erase(registry.characters.begin() + static_cast<std::ptrdiff_t>(i));
        if (registry.focusedId == id) registry.focusedId = INVALID_ENTITY;
        if (registry.activeDialogueId == id) {
            registry.activeDialogueId = INVALID_ENTITY;
            registry.dialogueOpen = false;
        }
        return;
    }
}

// ─── Lookup ──────────────────────────────────────────────────────────────────

InteractableCharacter* FindCharacter(EntityRegistry& registry, EntityId id) {
    for (std::size_t i = 0; i < registry.characterIds.size(); ++i) {
        if (registry.characterIds[i] == id) return &registry.characters[i];
    }
    return nullptr;
}

const InteractableCharacter* FindCharacter(const EntityRegistry& registry, EntityId id) {
    for (std::size_t i = 0; i < registry.characterIds.size(); ++i) {
        if (registry.characterIds[i] == id) return &registry.characters[i];
    }
    return nullptr;
}

int CharacterCount(const EntityRegistry& registry) {
    return static_cast<int>(registry.characters.size());
}

InteractableCharacter& CharacterAt(EntityRegistry& registry, int index) {
    return registry.characters[static_cast<std::size_t>(index)];
}

const InteractableCharacter& CharacterAt(const EntityRegistry& registry, int index) {
    return registry.characters[static_cast<std::size_t>(index)];
}

EntityId CharacterIdAt(const EntityRegistry& registry, int index) {
    return registry.characterIds[static_cast<std::size_t>(index)];
}

int IndexOfCharacter(const EntityRegistry& registry, EntityId id) {
    for (std::size_t i = 0; i < registry.characterIds.size(); ++i) {
        if (registry.characterIds[i] == id) return static_cast<int>(i);
    }
    return -1;
}

// ─── Gameplay operations ──────────────────────────────────────────────────────

void UpdateEntityFocus(EntityRegistry& registry, const Camera& camera, Vector3 playerPosition, float interactionDistance, float rayLength) {
    registry.focusedId = INVALID_ENTITY;
    if (registry.dialogueOpen) return;

    Ray ray = Ray{camera.position, Vector3Normalize(Vector3Subtract(camera.target, camera.position))};
    float bestDistance = rayLength;
    for (std::size_t i = 0; i < registry.characters.size(); ++i) {
        const InteractableCharacter& character = registry.characters[i];
        for (std::size_t c = 0; c < character.colliders.size(); ++c) {
            const CharacterCapsule& capsule = character.colliders[c];
            Vector3 center = Vector3Scale(Vector3Add(capsule.bottom, capsule.top), 0.5f);
            float dx = playerPosition.x - center.x;
            float dz = playerPosition.z - center.z;
            if (sqrtf(dx * dx + dz * dz) > interactionDistance + capsule.radius) continue;

            float aimDistance = DistanceRayToCapsuleApprox(ray, capsule) - capsule.radius;
            float cameraDistance = Vector3Distance(camera.position, center);
            if (aimDistance <= 0.25f && cameraDistance <= rayLength && cameraDistance < bestDistance) {
                bestDistance = cameraDistance;
                registry.focusedId = registry.characterIds[i];
            }
        }
    }
}

void BeginFocusedDialogue(EntityRegistry& registry) {
    if (registry.focusedId == INVALID_ENTITY) return;
    registry.dialogueOpen = true;
    registry.activeDialogueId = registry.focusedId;
    registry.selectedChoiceIndex = 0;
}

void UpdateDialogueInput(EntityRegistry& registry, const InputMap& inputMap) {
    if (!registry.dialogueOpen) return;
    const InteractableCharacter* character = FindCharacter(registry, registry.activeDialogueId);
    if (!character) { registry.dialogueOpen = false; return; }

    int count = static_cast<int>(character->choices.size());
    if (count > 0) {
        bool navigateDown = IsActionPressed(inputMap, GameAction::MoveBack)    || IsKeyPressed(KEY_DOWN);
        bool navigateUp   = IsActionPressed(inputMap, GameAction::MoveForward) || IsKeyPressed(KEY_UP);
        if (navigateDown) registry.selectedChoiceIndex = (registry.selectedChoiceIndex + 1) % count;
        if (navigateUp)   registry.selectedChoiceIndex = (registry.selectedChoiceIndex + count - 1) % count;
    }
    if (IsActionPressed(inputMap, GameAction::Interact) || IsKeyPressed(KEY_SPACE)) {
        registry.dialogueOpen = false;
        registry.activeDialogueId = INVALID_ENTITY;
        registry.selectedChoiceIndex = 0;
    }
}

bool ResolveCharacterCollisions(EntityRegistry& registry, Vector3& playerPosition, float playerRadius) {
    bool movedPlayer = false;
    for (std::size_t i = 0; i < registry.characters.size(); ++i) {
        const InteractableCharacter& character = registry.characters[i];
        for (std::size_t c = 0; c < character.colliders.size(); ++c) {
            const CharacterCapsule& capsule = character.colliders[c];
            Vector2 a = Vector2{capsule.bottom.x, capsule.bottom.z};
            Vector2 b = Vector2{capsule.top.x, capsule.top.z};
            Vector2 p = Vector2{playerPosition.x, playerPosition.z};
            Vector2 ab = Vector2Subtract(b, a);
            float abLenSqr = Vector2LengthSqr(ab);
            float t = abLenSqr > 0.0001f
                ? Clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / abLenSqr, 0.0f, 1.0f)
                : 0.0f;
            Vector2 closest = Vector2Add(a, Vector2Scale(ab, t));
            Vector2 delta = Vector2Subtract(p, closest);
            float minDistance = playerRadius + capsule.radius;
            float distSqr = Vector2LengthSqr(delta);
            if (distSqr > minDistance * minDistance) continue;
            if (distSqr > 0.0001f) {
                float dist = sqrtf(distSqr);
                float push = minDistance - dist;
                playerPosition.x += (delta.x / dist) * push;
                playerPosition.z += (delta.y / dist) * push;
                movedPlayer = true;
            } else {
                playerPosition.z += minDistance;
                movedPlayer = true;
            }
        }
    }
    return movedPlayer;
}

// ─── Rendering ───────────────────────────────────────────────────────────────

void DrawEntityCharacters(const EntityRegistry& registry) {
    int focusedIndex = IndexOfCharacter(registry, registry.focusedId);
    for (std::size_t i = 0; i < registry.characters.size(); ++i) {
        const InteractableCharacter& character = registry.characters[i];
        if (!character.hasModel) continue;

        for (std::size_t p = 0; p < character.visualParts.size(); ++p) {
            const CharacterRenderPart& part = character.visualParts[p];
            if (part.meshIndex < 0 || part.meshIndex >= character.model.meshCount) continue;
            int materialIndex = character.model.meshMaterial ? character.model.meshMaterial[part.meshIndex] : 0;
            materialIndex = Clamp(materialIndex, 0, character.model.materialCount - 1);
            DrawMesh(character.model.meshes[part.meshIndex], character.model.materials[materialIndex], part.transform);
        }

        if (focusedIndex == static_cast<int>(i)) {
            for (std::size_t p = 0; p < character.iconParts.size(); ++p) {
                const CharacterRenderPart& part = character.iconParts[p];
                if (part.meshIndex < 0 || part.meshIndex >= character.model.meshCount) continue;
                int materialIndex = character.model.meshMaterial ? character.model.meshMaterial[part.meshIndex] : 0;
                materialIndex = Clamp(materialIndex, 0, character.model.materialCount - 1);
                DrawMesh(character.model.meshes[part.meshIndex], character.model.materials[materialIndex], part.transform);
            }
        }
    }
}

void DrawInteractionUi(const EntityRegistry& registry) {
    if (!registry.dialogueOpen) {
        if (registry.focusedId != INVALID_ENTITY) {
            const char* prompt = "E Interagir";
            int width = MeasureUiText(prompt, 22);
            DrawRectangle(GetScreenWidth() / 2 - width / 2 - 14, GetScreenHeight() - 95, width + 28, 36, Color{0, 0, 0, 150});
            DrawUiText(prompt, GetScreenWidth() / 2 - width / 2, GetScreenHeight() - 88, 22, RAYWHITE);
        }
        return;
    }

    const InteractableCharacter* character = FindCharacter(registry, registry.activeDialogueId);
    if (!character) return;

    int margin = 44;
    int choiceCount = static_cast<int>(character->choices.size());
    int visibleChoiceRows = std::min(choiceCount, 5);
    int boxHeight = std::min(GetScreenHeight() - margin * 2, std::max(190, 170 + visibleChoiceRows * 24));
    Rectangle panel = Rectangle{
        static_cast<float>(margin),
        static_cast<float>(GetScreenHeight() - boxHeight - margin),
        static_cast<float>(GetScreenWidth() - margin * 2),
        static_cast<float>(boxHeight)};
    DrawRectangleRec(panel, Color{18, 18, 24, 225});
    DrawRectangleLinesEx(panel, 2.0f, Color{220, 220, 230, 220});

    Rectangle portrait = Rectangle{panel.x + 24.0f, panel.y + 28.0f, 104.0f, 104.0f};
    DrawRectangleRec(portrait, Color{55, 55, 70, 255});
    DrawRectangleLinesEx(portrait, 2.0f, Color{160, 160, 180, 255});
    DrawUiText("IMG", static_cast<int>(portrait.x + 31), static_cast<int>(portrait.y + 40), 24, LIGHTGRAY);

    int textX = static_cast<int>(panel.x + 154.0f);
    int textY = static_cast<int>(panel.y + 28.0f);
    DrawUiText(character->displayName.c_str(), textX, textY, 26, RAYWHITE);
    WrapAndDrawUiText(character->dialogueText, textX, textY + 42, 21, static_cast<int>(panel.width - 190.0f), LIGHTGRAY);

    int choiceY = static_cast<int>(panel.y + panel.height - 32.0f - visibleChoiceRows * 24);
    if (!character->choices.empty()) {
        int firstChoice = 0;
        if (choiceCount > visibleChoiceRows) {
            firstChoice = std::max(0, std::min(
                registry.selectedChoiceIndex - visibleChoiceRows / 2,
                choiceCount - visibleChoiceRows));
        }
        for (int row = 0; row < visibleChoiceRows; ++row) {
            int choiceIndex = firstChoice + row;
            Color color = registry.selectedChoiceIndex == choiceIndex ? YELLOW : RAYWHITE;
            DrawUiText(
                TextFormat("%s %s",
                    registry.selectedChoiceIndex == choiceIndex ? ">" : " ",
                    character->choices[choiceIndex].text.c_str()),
                textX, choiceY, 20, color);
            choiceY += 24;
        }
        if (choiceCount > visibleChoiceRows) {
            DrawUiText(
                TextFormat("%d/%d", registry.selectedChoiceIndex + 1, choiceCount),
                static_cast<int>(panel.x + panel.width - 70.0f),
                static_cast<int>(panel.y + panel.height - 28.0f),
                16, GRAY);
        }
    } else {
        DrawUiText("E continuar",
            static_cast<int>(panel.x + panel.width - 140.0f), choiceY, 18, GRAY);
    }
}
